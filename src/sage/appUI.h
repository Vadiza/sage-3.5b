/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appUI.h
 * Author : Ratko Jagodic
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef APPUI_H_
#define APPUI_H_

#include "appWidgets.h"
#include <map>
#include <string.h>
#include "sail.h"
#include <string>

class appWidget;
class sail;

using namespace std;

class appUI {

private:
  // key=widgetID, value=pointer to widget
  map<const int, appWidget *> widgets;
  int getNewId();
  int lastId;
  sail *sailObj;
  int bufferW;  // app buffer size to calculate normalized coords
  int bufferH;
  bool initialized;

  void sendWidgetInfo(appWidget *w);

public:
  appUI(sail *s);
  ~appUI();
  void addWidget(appWidget *w);
  void removeWidget(appWidget *w);
  void processWidgetEvent(const char *eventData);

  // called by sail to send the status of all widgets
  // to fsManager... includes data such as size and pos
  void sendInitialWidgetInfo();
};


#endif
