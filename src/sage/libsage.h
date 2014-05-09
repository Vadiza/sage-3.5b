/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sage.h
 * Author : Luc Renambot
 *
 * Copyright (C) 2012 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc about SAGE to the forums on
 * http://sagecommons.org/
 *
 *****************************************************************************/

#ifndef LIBSAGE_H
#define LIBSAGE_H


// headers for SAGE
#include "sail.h"
#include "appWidgets.h"
#include "misc.h"

typedef void(*sageQuitFunc)(sail*);
typedef void(*sageWidgetFunc)(sail*);
typedef void(*sageSyncFunc)(sail*);

typedef struct _application_update {
  int updated;
  int app_id;
  int wall_width;
  int wall_height;
  int app_x, app_y, app_w, app_h;
} application_update_t;

// Create and initialize a SAIL object
sail*          createSAIL(const char *appname, int ww, int hh, enum sagePixFmt pixelfmt, const char *fsIP, int roworder = BOTTOM_TO_TOP, int frate = 60, sageWidgetFunc wFunc = NULL);

// Disconnect and delete a SAIL object
void           deleteSAIL(sail *sageInf);

// Return the next buffer to be filled
unsigned char* nextBuffer(sail *sageInf);

// Swap buffers and stream
void           swapBuffer(sail *sageInf);

// Swap buffers and return the next buffer to be filled
unsigned char* swapAndNextBuffer(sail *sageInf);

// Fill the next buffer and swap
void           swapWithBuffer(sail *sageInf, unsigned char *pixptr);

// Process SAGE message from fsManager and DIM
void           processMessages(sail *sageInf, application_update_t *up, sageQuitFunc qfunc, sageSyncFunc sfunc);


// blocking call that returns the wall size (return 1/0 in case success/fail)
int            getWallSize(int &width, int &height);

// Create a application copy of itself
void           replicateClient(sail *sageInf);



#endif // LIBSAGE_H
