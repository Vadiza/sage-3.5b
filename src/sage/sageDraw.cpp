/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageDraw.cpp
 * Author : Byungil Jeong
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

#include "sageDraw.h"
#include "overlayApp.h"
#include "overlayPointer.h"
#include "overlayButton.h"
#include "overlayIcon.h"
#include "overlayLabel.h"
#include "overlayMenu.h"
#include "overlaySizer.h"
#include "overlayPanel.h"
#include "overlaySplitter.h"
#include "overlayThumbnail.h"
#include "overlayEnduranceThumbnail.h"


sageDraw::sageDraw(bool &dirty, sageDisplayConfig cfg) : dirtyBit(dirty), dispCfg(cfg)
{
  dispCfg = cfg;
  displayID = cfg.displayID;
  objList.clear();
  //sage::initUtil();
  sageDrawObject::initFonts();
  doShowSizers = 0;
}


sageDraw::~sageDraw()
{
  std::map<const int, sageDrawObject *>::iterator it;
  for (it=objList.begin(); it!=objList.end(); it++)
    delete (*it).second;
  objList.clear();
}


sageDrawObject* sageDraw::createDrawObject(char *name)
{
  sageDrawObject *newObj = NULL;

  if (strcmp(name, "pointer") == 0)
    newObj = (sageDrawObject *)new overlayPointer;
  else if (strcmp(name, "app") == 0)
    newObj = (sageDrawObject *)new overlayApp;
  else if (strcmp(name, "button") == 0)
    newObj = (sageDrawObject *)new overlayButton;
  else if (strcmp(name, "icon") == 0)
    newObj = (sageDrawObject *)new overlayIcon;
  else if (strcmp(name, "label") == 0)
    newObj = (sageDrawObject *)new overlayLabel;
  else if (strcmp(name, "menu") == 0)
    newObj = (sageDrawObject *)new overlayMenu;
  else if (strcmp(name, "sizer") == 0)
    newObj = (sageDrawObject *)new overlaySizer;
  else if (strcmp(name, "splitter") == 0)
    newObj = (sageDrawObject *)new overlaySplitter;
  else if (strcmp(name, "panel") == 0)
    newObj = (sageDrawObject *)new overlayPanel;
  else if (strcmp(name, "thumbnail") == 0)
    newObj = (sageDrawObject *)new overlayThumbnail;
  else if (strcmp(name, "enduranceThumbnail") == 0)
    newObj = (sageDrawObject *)new overlayEnduranceThumbnail;

  return newObj;
}

void sageDraw::update()
{
  double currentTime = sage::getTime();  // in microsecs

  // update all overlays... if they need updates
  std::map<const int, sageDrawObject *>::iterator iter;
  for (iter = objList.begin(); iter != objList.end(); iter++)
    if ((*iter).second)
      (*iter).second->update(currentTime);

  // set the creation time of this appInfo object (used for montage alphas)
  std::map<const int, appInfo>::iterator it;
  for (it = apps.begin(); it != apps.end(); it++)
  {
    if ((*it).second.timeCreated == 0.0)
      (*it).second.timeCreated = currentTime;
  }

  sageDrawObject::selUpdated = false;
}

void sageDraw::setDirty()
{
  dirtyBit=true;
}


// Draw the parents that are of the specified drawOrder and
// that are related to specified winID.
// winID is optional and if not specified, all of the parent of this
// drawOrder are considered for drawing. (only objects that don't have
// parents are drawn because parents will draw their children in order
// for them to be drawn in the correct z order... needed for transparency)

void sageDraw::draw(sageRect rect, int drawOrder, int winID)
{
  std::vector<sageDrawObject*>menuObjects;

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  // loop through all the widgets, find the ones with the
  // corresponding drawOrder and draw them
  std::map<const int, sageDrawObject *>::iterator iter;
  for (iter = objList.begin(); iter != objList.end(); iter++) {

    // if we are drawing global widgets, remember to draw all
    // the menus because they should be drawn on top of everything
    if (drawOrder == SAGE_INTER_DRAW && winID == -1 &&
        (*(*iter).second) == MENU)
      menuObjects.push_back((*iter).second);

    // draw the object if it satisfies all the criteria
    if ((*iter).second &&
        (*iter).second->visible &&
        displayID==(*iter).second->displayID &&
        (*iter).second->drawOrder == drawOrder &&
        (winID == (*iter).second->winID) &&
        (*iter).second->parentID == -1)   // top-level parents only
    {

      //(*iter).second->setViewport(rect);
      (*iter).second->redraw();
      glColor4f(1.0, 1.0, 1.0, 1.0);   // reset the color
    }
  }

  // now draw all the delayed objects
  while (!delayedDrawQueue.empty()) {
    delayedDrawQueue.front()->redraw(true);
    delayedDrawQueue.pop();
  }

  // finally draw the menus
  if (!menuObjects.empty())
    for(int i=0; i<menuObjects.size(); i++)
      ((overlayMenu*)menuObjects[i])->drawMenu();


  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

}


void sageDraw::delayDraw(sageDrawObject *obj)
{
  delayedDrawQueue.push(obj);
}

// add object instances to be drawn in run-time
int sageDraw::addObjectInstance(char *data)
{
  char objectType[SAGE_NAME_LEN];
  int objID, widgetID, winID;

  // parse the first part before the xml document
  sscanf(data, "%d %s %d %d", &objID, &objectType, &widgetID, &winID);

  // parse the beginning of the xml document
  TiXmlDocument xml;
  xml.Parse(sage::tokenSeek(data, 4));
  if (xml.Error()) {
    SAGE_PRINTLOG("\n\nError while parsing drawObject XML: %s\n",xml.ErrorDesc());
    return 1;
  }

  // create the draw object based on the type
  sageDrawObject *newInst = createDrawObject(objectType);
  if (!newInst)  {
    SAGE_PRINTLOG("\n\nNo object created for type: %s\n", objectType);
    return 1;
  }

  // the object will parse the rest of the info from xml
  newInst->parseXml(xml.RootElement());
  newInst->widgetID = widgetID;
  newInst->winID = winID;
  newInst->init(objID, this, objectType);

  //if (winID > -1)
  //SAGE_PRINTLOG("===>>>> ADDING DISP OBJECT: widgetID %d, winID %d\n", newInst->widgetID, newInst->winID);

  // if the object is tied to an app, set its appBounds, figure out
  // the position relative to the app and initialize it...
  // otherwise, we already applied its position above so just update bounds
  if (newInst->winID != -1) {   // tied to an app?
    if (apps.count(newInst->winID) > 0) {   // we have app info already?
      appInfo a = apps[newInst->winID];
      a.minimized = false;
      a.mouseOver = false;
      if (a.x!=0 && a.y!=0 && a.w!=0 && a.h!=0)
        newInst->updateParentBounds(a.x, a.y, a.w, a.h, a.rot);
      newInst->updateParentDepth(a.z);
    }
  }
  else   // not tied to an app...
    newInst->updateBounds();

  // add to a parent if necessary
  int doAdd = 1;
  if (newInst->parentID != -1)
    doAdd = addToParent(newInst);

  // add the object to the draw list
  if (doAdd == 1) {
    objList[objID] = newInst;

    // maybe just-added object is somebody's parent so add
    // previously arrived children
    checkObjectQueue();
  }
  else
    objQueue.insert(newInst);

  return 0;
}


// queued widgets are somebody's children but that parent hasn't arrived
// yet so check for the parent now
void sageDraw::checkObjectQueue()
{
  int numAdded;
  std::set<sageDrawObject *>::iterator it;

  // in first iteration add children, then in the next one add their
  // children and so on until nothing was added
  do {
    numAdded = 0;
    for (it = objQueue.begin(); it != objQueue.end();) {
      if (addToParent(*it) == 1) {
        objList[(*it)->getID()] = *it;
        objQueue.erase(it++);
        numAdded++;
      }
      else
        it++;
    }
  } while(numAdded > 0);
}


int sageDraw::addToParent(sageDrawObject* drawObj)
{
  // if this is app-created widget, convert app-specific parentID to objID
  int objID = -1;
  std::map<const int, sageDrawObject *>::iterator iter;
  for (iter = objList.begin(); iter != objList.end(); iter++) {
    if ((*iter).second->widgetID == drawObj->parentID &&
        (*iter).second->winID == drawObj->winID) {
      objID = (*iter).first;
      drawObj->parentID = objID; // modify the parentID to objID of the parent
      break;
    }
  }

  // finally add the widget to parent if the right parent was found
  if (objList.count(objID) > 0) {
    objList[objID]->addChild(drawObj);
    return 1;
  }
  else
    return 0;
}


sageDrawObject* sageDraw::getDrawObject(int id)
{
  if (objList.count(id) > 0)
    return objList[id];
  else
    return NULL;
}

sageDrawObject* sageDraw::getAppOverlay(int appId)
{
  std::map<const int, sageDrawObject *>::iterator iter;
  for (iter = objList.begin(); iter != objList.end(); iter++) {
    if ((*iter).second && (*iter).second->winID == appId && (*(*iter).second)==APP)
      return (*iter).second;
  }
  return NULL;
}

int sageDraw::removeObject(int id)
{
  // find the object we are removing and erase it
  if (objList.count(id) > 0) {
    sageDrawObject *obj = objList[id];
    if (obj->parentID != -1 &&   // if the obj is a part of a parent
        objList.count(obj->parentID)>0)
      objList[obj->parentID]->removeChild(id);
    delete obj;
    objList.erase(id);
  }
  return 0;
}

void sageDraw::onAppShutdown(int winID)
{
  apps.erase(winID);
}


int sageDraw::showObject(char *data)
{
  int id = -1;
  int doShow = 1;
  sscanf(data, "%d %d", &id, &doShow);

  // find the object we are changing and update it
  if (objList.count(id) > 0)
    objList[id]->showObject(bool(doShow));

  return 0;
}


int sageDraw::forwardObjectMessage(char *data)
{
  int objId, msgCode;

  // loop through the whole message, split it by newlines
  // and send them off to the right objects

  char *oneMessage = strtok(data, "\n");

  while (oneMessage != NULL) {
    sscanf(oneMessage, "%d %d", &objId, &msgCode);

    if (objList.count(objId) > 0) {
      // common messages have msgCode < 0 and widget
      // specific messages have positive msg codes
      if(msgCode == SET_APP_BOUNDS)
      {
        int winID;
        sscanf(sage::tokenSeek(oneMessage, 2), "%d", &winID);
        resetApp(winID);
      }
      else if(msgCode == SET_INITIAL_BOUNDS)
      {
        int winID, x, y, w, h, r;
        float z;
        sscanf(sage::tokenSeek(oneMessage, 2), "%d %d %d %d %d %d %f", &winID, &x, &y, &w, &h, &r, &z);
        updateInitialBounds(winID, x, y, w, h, (sageRotation)r, z);
      }
      else if(msgCode == SHOW_SIZERS)
        sscanf(sage::tokenSeek(oneMessage, 2), "%d", &doShowSizers);
      else if (msgCode >= 0 && msgCode <= COMMON_WIDGET_EVENTS)
        objList[objId]->parseMessage( oneMessage );
      else
        objList[objId]->parseCommonMessage( oneMessage );
    }
    oneMessage = strtok(NULL, "\n");
  }

  return 0;
}


void sageDraw::updateAppBounds(int winID, int x, int y, int w, int h, sageRotation orientation)
{
  // remember each app's bounds
  apps[winID].x = x;
  apps[winID].y = y;
  apps[winID].w = w;
  apps[winID].h = h;
  apps[winID].rot = orientation;
  apps[winID].mouseOver = false;

  std::map<const int, sageDrawObject *>::iterator iter;

  // find the object (if any) that is tied to this app and update it
  for (iter = objList.begin(); iter != objList.end(); iter++)
    if ((*iter).second &&
        (*iter).second->winID == winID && // tied to this app
        (*iter).second->parentID == -1)    // top level parents only
      (*iter).second->updateParentBounds(x, y, w, h, orientation);
}


void sageDraw::updateAppDepth(int winID, float depth)
{
  // remember this app's info
  apps[winID].z = depth;

  // find the object (if any) that is tied to this app and update it
  std::map<const int, sageDrawObject *>::iterator iter;

  for (iter = objList.begin(); iter != objList.end(); iter++)
    if ((*iter).second && (*iter).second->winID == winID &&
        (*iter).second->parentID == -1)
      (*iter).second->updateParentDepth(depth);
}


void sageDraw::resetApp(int winID)
{
  if (apps.count(winID) == 0)
    return;

  // find the object (if any) that is tied to this app and update it
  std::map<const int, sageDrawObject *>::iterator iter;
  for (iter = objList.begin(); iter != objList.end(); iter++) {
    if ((*iter).second && (*iter).second->winID == winID &&
        (*iter).second->parentID == -1)
    {
      (*iter).second->updateParentDepth(apps[winID].z);
      (*iter).second->updateParentBounds(apps[winID].x,
                                         apps[winID].y,
                                         apps[winID].w,
                                         apps[winID].h,
                                         apps[winID].rot);
    }
  }
}


void sageDraw::updateInitialBounds(int winID, int x, int y, int w, int h, sageRotation orientation, float depth)
{
  // remember each app's bounds
  apps[winID].x = x;
  apps[winID].y = y;
  apps[winID].w = w;
  apps[winID].h = h;
  apps[winID].z = depth;
  apps[winID].rot = orientation;
  apps[winID].mouseOver = false;;

  std::map<const int, sageDrawObject *>::iterator iter;

  // find the object (if any) that is tied to this app and update it
  for (iter = objList.begin(); iter != objList.end(); iter++)
    if ((*iter).second &&
        (*iter).second->winID == winID && // tied to this app
        (*iter).second->parentID == -1)  {    // top level parents only
      (*iter).second->updateParentBounds(x, y, w, h, orientation);
      (*iter).second->updateParentDepth(depth);
    }
}



void sageDraw::minimizeApp(int winID, bool minimized)
{
  apps[winID].minimized = minimized;
}
