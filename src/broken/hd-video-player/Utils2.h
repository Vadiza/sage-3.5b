/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Copyright (C) 2004 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/


#ifndef _UTIL2_
#define _UTIL2_

#include <dvdmedia.h>

HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath);

class C_SaveToLog {
  FILE *fp;
  char fileName[125];

  int index;
  char string[1024];

public :
  C_SaveToLog(char*str = NULL) {
    if(str == NULL)
      strcpy(fileName, "log.txt");
    else
      strcpy(fileName, str);

    fp = fopen(fileName, "w");

    fprintf(fp, "<Log file> \n ");

    index = 0;
  }

  ~C_SaveToLog() {
    if(fp)
      fclose(fp);
  }

  void ToLog(char* str) {
    if(fp)
      fprintf(fp, "%s", str);
  }

  void ToLog(int input) {
    if(fp)
      fprintf(fp, "%d", input);
  }

  void ToLog(char* str, int len) {
    if(fp) {
      char tmp[1024];
      strncpy(tmp, str, len);
      strcat(tmp, "");
      fprintf(fp, "\n Len(%d) : %s", len, tmp);
    }
  }

  void ToLog16(char* str, int len) {
    if(fp) {
      char tmp[1024];
      strncpy(tmp, str, len);
      strcat(tmp, "");
      fprintf(fp, "\n Len(%d) : %d", len, tmp);
      for(int i = 0; i < len; i++)
        fprintf(fp, "\n %x", tmp[i]);
    }
  }

  void CleanStr(void) {
    index = 0;
  }

  void ToStr(char* str) {
    strcpy(&string[index], str);
    index += strlen(string);
  }

  void ToStr(int input) {
    char str[100];

    itoa(input, str, 10);
    strcpy(&string[index], str);
    index += strlen(string);
  }

  void SaveStr(void) {
    if(fp)
      fprintf(fp, "\n%s", string);
  }
};


#ifdef _MAIN_
class C_SaveToLog SaveLog(NULL);
#else
extern class C_SaveToLog SaveLog;
#endif


#endif
