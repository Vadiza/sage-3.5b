/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <ft2build.h>
#include FT_GLYPH_H
#include <strings.h>

#include "ass_library.h"
#include "ass.h"
#include "ass_utils.h"

int mystrtoi(char **p, int *res)
{
  double temp_res;
  char *start = *p;
  temp_res = ass_strtod(*p, p);
  *res = (int) (temp_res + (temp_res > 0 ? 0.5 : -0.5));
  if (*p != start)
    return 1;
  else
    return 0;
}

int mystrtoll(char **p, long long *res)
{
  double temp_res;
  char *start = *p;
  temp_res = ass_strtod(*p, p);
  *res = (int) (temp_res + (temp_res > 0 ? 0.5 : -0.5));
  if (*p != start)
    return 1;
  else
    return 0;
}

int mystrtou32(char **p, int base, uint32_t *res)
{
  char *start = *p;
  *res = strtoll(*p, p, base);
  if (*p != start)
    return 1;
  else
    return 0;
}

int mystrtod(char **p, double *res)
{
  char *start = *p;
  *res = ass_strtod(*p, p);
  if (*p != start)
    return 1;
  else
    return 0;
}

int strtocolor(ASS_Library *library, char **q, uint32_t *res, int hex)
{
  uint32_t color = 0;
  int result;
  char *p = *q;
  int base = hex ? 16 : 10;

  if (*p == '&')
    ++p;
  else
    ass_msg(library, MSGL_DBG2, "suspicious color format: \"%s\"\n", p);

  if (*p == 'H' || *p == 'h') {
    ++p;
    result = mystrtou32(&p, 16, &color);
  } else {
    result = mystrtou32(&p, base, &color);
  }

  {
    unsigned char *tmp = (unsigned char *) (&color);
    unsigned char b;
    b = tmp[0];
    tmp[0] = tmp[3];
    tmp[3] = b;
    b = tmp[1];
    tmp[1] = tmp[2];
    tmp[2] = b;
  }
  if (*p == '&')
    ++p;
  *q = p;

  *res = color;
  return result;
}

// Return a boolean value for a string
char parse_bool(char *str)
{
  while (*str == ' ' || *str == '\t')
    str++;
  if (!strncasecmp(str, "yes", 3))
    return 1;
  else if (strtol(str, NULL, 10) > 0)
    return 1;
  return 0;
}

void ass_msg(ASS_Library *priv, int lvl, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  priv->msg_callback(lvl, fmt, va, priv->msg_callback_data);
  va_end(va);
}

unsigned ass_utf8_get_char(char **str)
{
  uint8_t *strp = (uint8_t *) * str;
  unsigned c = *strp++;
  unsigned mask = 0x80;
  int len = -1;
  while (c & mask) {
    mask >>= 1;
    len++;
  }
  if (len <= 0 || len > 4)
    goto no_utf8;
  c &= mask - 1;
  while ((*strp & 0xc0) == 0x80) {
    if (len-- <= 0)
      goto no_utf8;
    c = (c << 6) | (*strp++ & 0x3f);
  }
  if (len)
    goto no_utf8;
  *str = (char *) strp;
  return c;

  no_utf8:
  strp = (uint8_t *) * str;
  c = *strp++;
  *str = (char *) strp;
  return c;
}

/**
 * \brief find style by name
 * \param track track
 * \param name style name
 * \return index in track->styles
 * Returnes 0 if no styles found => expects at least 1 style.
 * Parsing code always adds "Default" style in the end.
 */
int lookup_style(ASS_Track *track, char *name)
{
  int i;
  if (*name == '*')
    ++name;                 // FIXME: what does '*' really mean ?
  for (i = track->n_styles - 1; i >= 0; --i) {
    if (strcmp(track->styles[i].Name, name) == 0)
      return i;
  }
  i = track->default_style;
  ass_msg(track->library, MSGL_WARN,
          "[%p]: Warning: no style named '%s' found, using '%s'",
          track, name, track->styles[i].Name);
  return i;                   // use the first style
}

#ifdef CONFIG_ENCA
void *ass_guess_buffer_cp(ASS_Library *library, unsigned char *buffer,
                          int buflen, char *preferred_language,
                          char *fallback)
{
  const char **languages;
  size_t langcnt;
  EncaAnalyser analyser;
  EncaEncoding encoding;
  char *detected_sub_cp = NULL;
  int i;

  languages = enca_get_languages(&langcnt);
  ass_msg(library, MSGL_V, "ENCA supported languages");
  for (i = 0; i < langcnt; i++) {
    ass_msg(library, MSGL_V, "lang %s", languages[i]);
  }

  for (i = 0; i < langcnt; i++) {
    const char *tmp;

    if (strcasecmp(languages[i], preferred_language) != 0)
      continue;
    analyser = enca_analyser_alloc(languages[i]);
    encoding = enca_analyse_const(analyser, buffer, buflen);
    tmp = enca_charset_name(encoding.charset, ENCA_NAME_STYLE_ICONV);
    if (tmp && encoding.charset != ENCA_CS_UNKNOWN) {
      detected_sub_cp = strdup(tmp);
      ass_msg(library, MSGL_INFO, "ENCA detected charset: %s", tmp);
    }
    enca_analyser_free(analyser);
  }

  free(languages);

  if (!detected_sub_cp) {
    detected_sub_cp = strdup(fallback);
    ass_msg(library, MSGL_INFO,
            "ENCA detection failed: fallback to %s", fallback);
  }

  return detected_sub_cp;
}
#endif
