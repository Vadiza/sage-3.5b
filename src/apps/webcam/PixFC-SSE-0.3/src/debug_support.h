/*
 * debug_support.h
 *
 * Copyright (C) 2011 PixFC Team (pixelfc@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public  License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DEBUG_SUPPORT_H_
#define DEBUG_SUPPORT_H_

#if defined(__linux__) || defined(__APPLE__)

#ifndef DEBUG
#define EXTERN_INLINE             extern inline
#else
#define EXTERN_INLINE
#endif
#define INLINE                  inline

#else

#ifndef DEBUG
#define EXTERN_INLINE             __forceinline
#else
#define EXTERN_INLINE
#endif
#define INLINE                  __inline

#endif



// Debug print
#ifndef DEBUG

#define dprint(...)

#else

#define dprint(fmt, ...)  do {                                          \
    fprintf (stderr, "[ %s:%d ]\t" fmt, __FILE__, __LINE__, ## __VA_ARGS__); \
    fflush(stderr);                                                     \
  } while(0)

#endif

#endif /* DEBUG_SUPPORT_H_ */