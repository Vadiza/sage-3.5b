/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: appUI.cpp
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


#include "appUI.h"

appUI::appUI(sail *s)
{
  // removed (luc): SDL_Init(SDL_INIT_VIDEO);
  sailObj = s;
  lastId = 0;
  initialized = false;
}

appUI::~appUI()
{
  // delete all the widgets
  std::map<const int, appWidget *>::iterator it;
  for (it=widgets.begin(); it!=widgets.end(); it++)
    DRAW_DELETE ((*it).second);
  widgets.clear();
}

void appUI::addWidget(appWidget *w)
{
  w->widgetID = ++lastId;
  w->sailObj = sailObj;
  widgets[w->widgetID] = w;

  if (initialized && !w->ui)
    sendWidgetInfo(w);

  w->ui = this;   // so that we can add children

  // parents add all their children
  if (w->hasChildren())
    w->addChildren();
}

// removes the widget and calls its destructor
// (therefore don't use the widget after calling this)
void appUI::removeWidget(appWidget *w)
{
  // if removing parent, remove all the children as well
  if (w->hasChildren()) {

    // go through all the widgets and delete the ones that are in this parent
    map<const int, appWidget *>::iterator it;
    for (it = widgets.begin(); it != widgets.end();) {
      if ((*it).second->parentID == w->getId()) {
        widgets.erase(it++);
        DRAW_DELETE((*it).second);
      }
      else
        it++;
    }
  }

  // now remove the widget itself
  w->destroy();
  if (widgets.count(w->getId()) > 0) {
    widgets.erase(w->getId());

    // if the widget is a part of a parent, remove it from there as well
    if (w->parentID != -1 && widgets.count(w->parentID) > 0)
      widgets[w->parentID]->removeWidget(w);
  }
  DRAW_DELETE(w);
}

void appUI::processWidgetEvent(const char *eventData)
{
  int widgetID, eventId;
  char eventMsg[512];

  sscanf(eventData, "%d %d %s", &eventId, &widgetID, eventMsg);

  // forward the event to the appropriate widget
  if (widgets.count(widgetID) > 0)
    widgets[widgetID]->processEvent(eventId, eventMsg);
}

void appUI::sendInitialWidgetInfo()
{
  // first get the buffer resolution from sail for converting
  // all the widgets' coordinates to normalized
  sailObj->getBufferResolution(bufferW, bufferH);

  // attach children to parents
  map<const int, appWidget *>::iterator it;
  for (it = widgets.begin(); it != widgets.end(); it++)
    (*it).second->setParentIDs();

  // loop through all the widgets and widgets and send their
  // status and event info to fsManager
  for (it = widgets.begin(); it != widgets.end(); it++)
    sendWidgetInfo((*it).second);

  // initialized means that the initial widget info has been sent to sage
  initialized = true;
}


void appUI::sendWidgetInfo(appWidget *w)
{
  ostringstream xmlString;  // to hold our xml doc as a string

  w->winID = sailObj->getWinID();  //used by getXML() so must be done 1st
  w->normalizeCoords(bufferW, bufferH);  // normalize coords

  //SAGE_PRINTLOG( "\n\nWidget = %s, id = %d, parentId = %d", w->getType(), w->widgetID, w->parentID);

  // convert xml document to string and send it off
  xmlString << w->getType() << " " << w->getId()
            << " " << sailObj->getWinID() << "\n" << w->getXML();
  sailObj->sendMessage(ADD_OBJECT, (char *)xmlString.str().c_str());
}
