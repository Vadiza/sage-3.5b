/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: tileConfig.cpp - Contains all class details/implementation
 *       for building a description of a tile display system, at run time
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

#include "tileConfig.h"
#include "sageConfig.h"

displayNode::displayNode() : dimX(0), dimY(0), port(0)
{
  dimX = 0, dimY = 0;
  memset((void *)ip, 0, SAGE_IP_LEN);
  streamIPs.clear();
  tiles.clear();
  sprintf(Xdisp, "0.0");
  winX = 0, winY = 0;
}

displayNode::~displayNode()
{
  for (int i=0; i<streamIPs.size(); i++) {
    if (streamIPs[i]) {
      delete [] streamIPs[i];
      streamIPs[i] = NULL;
    }
  }
  streamIPs.clear();

  for (int i=0; i<tiles.size(); i++) {
    tiles[i] = NULL;
  }
  tiles.clear();
}

int displayNode::computeDimension()
{
  dimX = dimY = 1;

  for (int i=1; i<tiles.size(); i++) {
    bool uniqueX = true, uniqueY = true;
    for (int j=0; j<i; j++) {
      uniqueX = uniqueX && (tiles[i]->xIndex != tiles[j]->xIndex);
      uniqueY = uniqueY && (tiles[i]->yIndex != tiles[j]->yIndex);
    }
    if (uniqueX)
      dimX++;
    if (uniqueY)
      dimY++;
  }

  return 0;
}

audioNode::audioNode() :
  sampleFmt(SAGE_SAMPLE_FLOAT32), samplingRate(44100), channels(2), framePerBuffer(512), deviceId(-1)
{
  memset((void *)ip, 0, SAGE_IP_LEN);
  streamIPs.clear();
  //tiles.clear();
}

audioNode::~audioNode()
{
  for (int i=0; i<streamIPs.size(); i++) {
    if (streamIPs[i]) {
      delete [] streamIPs[i];
      //free streamIPs[i];
      streamIPs[i] = NULL;
    }
  }
  streamIPs.clear();
}


virtualDesktop::virtualDesktop() : dimX(0), dimY(0), table(false), audioServer(false)
{
  tileList.clear();
  displayCluster.clear();
  audioCluster.clear();
  sprintf(globalType.name, "global");
  memset(masterIP, 0, SAGE_IP_LEN);
  masterIP[0] = '\0';
  audioDir[0] = '\0';
}

int virtualDesktop::updateDesktop()
{
  int *xOffset = new int[dimY];
  int *yOffset = new int[dimX];

  for (int yIdx = 0; yIdx < dimY; yIdx++)
    xOffset[yIdx] = x;

  for (int xIdx = 0; xIdx < dimX; xIdx++) {
    yOffset[xIdx] = y;

    for (int i=0; i<tileList.size(); i++) {
      if (tileList[i]->xIndex == xIdx) {
        int leftMullion = (int)floor(tileList[i]->tileType->left*tileList[i]->tileType->PPI+0.5);
        int rightMullion = (int)floor(tileList[i]->tileType->right*tileList[i]->tileType->PPI+0.5);

        if (xIdx > 0)
          xOffset[tileList[i]->yIndex] += leftMullion;

        //xOffset[tileList[i]->yIndex] += tileList[i]->offsetX;

        tileList[i]->x = xOffset[tileList[i]->yIndex];

        if (xIdx < dimX-1)
          xOffset[tileList[i]->yIndex] += tileList[i]->width + rightMullion;
        else
          xOffset[tileList[i]->yIndex] += tileList[i]->width;
      }
    }
  }

  for (int yIdx = 0; yIdx < dimY; yIdx++) {
    for (int i=0; i<tileList.size(); i++) {
      if (tileList[i]->yIndex == yIdx) {
        int topMullion = (int)floor(tileList[i]->tileType->top*tileList[i]->tileType->PPI+0.5);
        int bottomMullion = (int)floor(tileList[i]->tileType->bottom*tileList[i]->tileType->PPI+0.5);

        if (yIdx > 0)
          yOffset[tileList[i]->xIndex] += bottomMullion;

        //yOffset[tileList[i]->xIndex] += tileList[i]->offsetY;

        tileList[i]->y = yOffset[tileList[i]->xIndex];

        if (yIdx < dimY-1)
          yOffset[tileList[i]->xIndex] += tileList[i]->height + topMullion;
        else
          yOffset[tileList[i]->xIndex] += tileList[i]->height;
      }
    }
  }

  width = xOffset[dimY-1];
  height = yOffset[dimX-1];

  for (int i=0; i<tileList.size(); i++) {
    tileList[i]->x += tileList[i]->offsetX;
    tileList[i]->y += tileList[i]->offsetY;
    (*(sageRect *)tileList[i]) /= (*(sageRect *)this);
  }
  return 0;
}

bool virtualDesktop::parseConfigfile(FILE *fp, bool configBegin)
{
  char token[TOKEN_LEN];
  displayNode *newNode = NULL;
  tileInfo *newTile = NULL;
  bool fileContinue = false;
  bool parseNodeInfo = false;

  while (getToken(fp, token) != EOF) {
    sage::toupper(token);
    if (strcmp(token, "TILEDISPLAY") == 0) {
      if (!configBegin)
        configBegin = true;
      else {
        fileContinue = true;
        break;
      }
    }
    else if (strcmp(token, "DIMENSIONS") == 0) {
      getToken(fp, token);
      dimX = atoi(token);
      getToken(fp, token);
      dimY = atoi(token);
    }
    else if (strcmp(token, "MULLIONS") == 0) {
      getToken(fp, token);
      globalType.left = (float)atof(token);
      getToken(fp, token);
      globalType.right = (float)atof(token);
      getToken(fp, token);
      globalType.bottom = (float)atof(token);
      getToken(fp, token);
      globalType.top = (float)atof(token);
    }
    else if (strcmp(token, "RESOLUTION") == 0) {
      getToken(fp, token);
      globalType.width = atoi(token);
      getToken(fp, token);
      globalType.height = atoi(token);
    }
    else if (strcmp(token, "PPI") == 0) {
      getToken(fp, token);
      globalType.PPI = atoi(token);
    }
    //else if (strcmp(token, "MASTER") == 0) {
    //   getToken(fp, masterIP);
    //}
    //else if (strcmp(token, "DISPLAYID") == 0) {
    //   getToken(fp, token);
    //   displayID = atoi(token);
    //}
    else if (strcmp(token, "TABLE") == 0) {
      getToken(fp, token);
      sage::toupper(token);
      table = (strcmp(token, "YES") == 0);
    }
    else if (strcmp(token, "DISPLAYNODE") == 0) {
      if (parseNodeInfo && newNode) {
        if (newNode->dimX*newNode->dimY == 0)
          newNode->computeDimension();

        displayCluster.push_back(newNode);
      }

      parseNodeInfo = true;
      newNode = new displayNode;
    }
    else if (strcmp(token, "IP") == 0) {
      getToken(fp, token);
      char *ipToken = strtok(token, ":");
      if (ipToken) {
        strcpy(newNode->ip, ipToken);
        ipToken = strtok(NULL, ":");
        if (ipToken) {
          newNode->port = atoi(ipToken);
          ipToken = strtok(NULL, ":");
          if (ipToken)
            strcpy(newNode->Xdisp, ipToken);
        }
      }
      else {
        SAGE_PRINTLOG("virtualDesktop::parseConfigfile : no IP address specified for a node");
        return false;
      }
    }
    else if (strcmp(token, "MONITORS") == 0) {
      getToken(fp, token);
      int tileNum = atoi(token);
      for (int i=0; i<tileNum; i++) {
        getToken(fp, token);
        if (strcmp(token, "(") != 0) {
          SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \"(\" expected");
          return false;
        }

        newTile = new tileInfo;
        getToken(fp, token);
        newTile->xIndex = atoi(token);
        getToken(fp, token);
        newTile->yIndex = atoi(token);

        getToken(fp, token);
        if (strcmp(token, ")") != 0) {
          newTile->offsetX = atoi(token);
          getToken(fp, token);
          newTile->offsetY = atoi(token);

          getToken(fp, token);
          if (strcmp(token, ")") != 0) {
            SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \")\" expected");
            return false;
          }
        }

        newTile->tileType = &globalType;
        newTile->nodeID = displayCluster.size();
        newTile->tileID = tileList.size();
        newTile->width = newTile->tileType->width;
        newTile->height = newTile->tileType->height;
        tileList.push_back(newTile);
        newNode->tiles.push_back(newTile);
      } // end for
    }
    else if (strcmp(token, "DIM") == 0) {
      getToken(fp, token);
      if (strcmp(token, "(") != 0) {
        SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \"(\" expected");
        return false;
      }

      getToken(fp, token);
      newNode->dimX = atoi(token);
      getToken(fp, token);
      newNode->dimY = atoi(token);

      getToken(fp, token);
      if (strcmp(token, ")") != 0) {
        SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \")\" expected");
        return false;
      }
    }
    else if (strcmp(token, "POS") == 0) {
      getToken(fp, token);
      if (strcmp(token, "(") != 0) {
        SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \"(\" expected");
        return false;
      }

      getToken(fp, token);
      newNode->winX = atoi(token);
      getToken(fp, token);
      newNode->winY = atoi(token);

      getToken(fp, token);
      if (strcmp(token, ")") != 0) {
        SAGE_PRINTLOG("virtualDesktop::parseConfigfile : invalid file format - \")\" expected");
        return false;
      }
    }
    else if (strcmp(token, "DISPLAY_CONNECTIONS") == 0) {
      fileContinue = false;
      break;
    }
  }

  if (parseNodeInfo) {
    if (newNode->dimX*newNode->dimY == 0)
      newNode->computeDimension();

    displayCluster.push_back(newNode);
  }

  updateDesktop();

  return fileContinue;
}

void virtualDesktop::locateApp(appInExec *app, displayConnection *connection, int idx)
{
  // a single table display, multiple vertical displays

  sageRect newLayout = *(sageRect *)app;

  if (connection) {
    sageRect neighbor = *(sageRect *)connection->displays[1-idx];
    int offset = connection->offset;

    if (table) {
      // vertical to table
      switch(connection->edges[idx]) {
      case LEFT_EDGE:
        newLayout.rotate(CCW_90);
        newLayout.x = 0;
        offset = connection->offset*(1-idx*2);
        newLayout.y = app->x - neighbor.halfWidth() + halfHeight() + offset;
        break;
      case RIGHT_EDGE:
        newLayout.rotate(CCW_270);
        newLayout.x = width - newLayout.width;
        newLayout.y = halfHeight() + offset - (app->x - neighbor.halfWidth())
          - newLayout.height;
        break;
      case BOTTOM_EDGE:
        newLayout.rotate(CCW_180);
        newLayout.x = halfWidth() + offset - (app->x - neighbor.halfWidth())
          - newLayout.width;
        newLayout.y = 0;
        break;
      case TOP_EDGE:
        offset = connection->offset*(1-idx*2);
        newLayout.x = halfWidth() + offset + (app->x - neighbor.halfWidth());
        newLayout.y = width - newLayout.height;
        break;
      }
    }
    else {
      // table to vertical
      if (connection->displays[1-idx]->table) {
        newLayout.resetOrientation();
        switch(connection->edges[1-idx]) {
        case LEFT_EDGE:
          offset = connection->offset*(1-idx*2);
          newLayout.x = halfWidth() + offset - newLayout.halfWidth()
            + app->centerY() - neighbor.halfHeight();
          break;
        case RIGHT_EDGE:
          newLayout.x = halfWidth() + offset - newLayout.halfWidth()
            - (app->centerY() - neighbor.halfHeight());
          break;
        case BOTTOM_EDGE:
          newLayout.x = halfWidth() + offset - newLayout.halfWidth()
            - (app->centerX() - neighbor.halfWidth());
          break;
        case TOP_EDGE:
          offset = connection->offset*(1-idx*2);
          newLayout.x = halfWidth() + offset - newLayout.halfWidth()
            + (app->centerX() - neighbor.halfWidth());
          break;
        }

        newLayout.y = 0;
      }
      else {
        // vertical to vertical
        switch(connection->edges[idx]) {
        case LEFT_EDGE:
          newLayout.x = 0;
          break;
        case RIGHT_EDGE:
          newLayout.x = width - newLayout.width;
          break;
        }

        offset = connection->offset*(1-idx*2);
        newLayout.y = halfHeight() + offset + app->y - neighbor.halfHeight();
      }
    }
  }

  // check if the app is out of bounds by more than half of its size...
  /*   if (newLayout.centerX() < 0)
       newLayout.x = 1 - newLayout.halfWidth();
       else if (newLayout.centerX() > width)
       newLayout.x = width - newLayout.halfWidth() - 1;
  */

  // NEW WAY!!!! allow up to 80% of the app to be out of the view on the display
  if (newLayout.x + (int) (newLayout.width*0.8) < 0)
    newLayout.x = -(int) (newLayout.width*0.8);
  else if (newLayout.x + (int) (newLayout.width*0.2) > width)
    newLayout.x = width - (int) (newLayout.width*0.2);


  if (newLayout.x < 0 && newLayout.x + newLayout.width > width) {
    float leftScale = (float)newLayout.centerX()/(newLayout.halfWidth()+1);
    float rightScale = (float)(width-newLayout.centerX())/(newLayout.halfWidth()+1);
    float scaleRatio = MAX(leftScale, rightScale);
    newLayout.scale(scaleRatio);
  }

  /*if (newLayout.centerY() < 0)
    newLayout.y = 1 - newLayout.halfHeight();
    else if (newLayout.centerY() > height)
    newLayout.y = height - newLayout.halfHeight() - 1;
  */


  // NEW WAY!!!! allow up to 80% of the app to be out of the view on the display
  if (newLayout.y + (int) (newLayout.height*0.8) < 0)
    newLayout.y = -(int) (newLayout.height*0.8);
  else if (newLayout.y + (int) (newLayout.height*0.2) > height)
    newLayout.y = height - (int) (newLayout.height*0.2);


  if (newLayout.y < 0 && newLayout.y + newLayout.height > height) {
    float bottomScale = (float)newLayout.centerY()/(newLayout.halfHeight()+1);
    float topScale = (float)(height-newLayout.centerY())/(newLayout.halfHeight()+1);
    float scaleRatio = MAX(bottomScale, topScale);
    newLayout.scale(scaleRatio);
  }

  if (connection) {
    switch(connection->edges[idx]) {
    case LEFT_EDGE:
      newLayout.x = 0;
      break;
    case RIGHT_EDGE:
      newLayout.x = width - newLayout.width;
      break;
    case BOTTOM_EDGE:
      newLayout.y = 0;
      break;
    case TOP_EDGE:
      newLayout.y = width - newLayout.height;
      break;
    }
  }

  *(sageRect *)app = newLayout;
}

int virtualDesktop::parseAudioConfigfile(FILE *fp, bool configBegin)
{
  char token[TOKEN_LEN];
  audioNode *newNode = NULL;
  bool fileContinue = false;
  bool parseNodeInfo = false;

  sageSampleFmt sampleFmt;
  long samplingRate;
  int  channels;
  int  framePerBuffer;
  int ip_length = SAGE_IP_LEN;
  char *attachedIP;
  int  deviceId=-1;

  while (getToken(fp, token) != EOF) {
    sage::toupper(token);
    if (strcmp(token, "AUDIOTILEDISPLAY") == 0) {
      if (!configBegin)
        configBegin = true;
      else {
        fileContinue = true;
        break;
      }
    }
    else if (strcmp(token, "SAMPLEFORMAT") == 0) {
      getToken(fp, token);
      sage::tolower(token);
      if (strcmp(token, "float32") == 0) {
        sampleFmt = SAGE_SAMPLE_FLOAT32;
      }
      else if (strcmp(token, "int16") == 0) {
        sampleFmt = SAGE_SAMPLE_INT16;
      }
      else if (strcmp(token, "int8") == 0) {
        sampleFmt = SAGE_SAMPLE_INT8;
      }
      else if (strcmp(token, "uint8") == 0) {
        sampleFmt = SAGE_SAMPLE_UINT8;
      }
    }
    else if (strcmp(token, "SAMPLINGRATE") == 0) {
      getToken(fp, token);
      samplingRate = atoi(token);
    }
    else if (strcmp(token, "CHANNELS") == 0) {
      getToken(fp, token);
      channels = atoi(token);
    }
    else if (strcmp(token, "FRAMEPERBUFFER") == 0) {
      getToken(fp, token);
      framePerBuffer = atoi(token);
    }
    else if (strcmp(token, "DEVICEID") == 0) {
      getToken(fp, token);
      deviceId = atoi(token);
    }
    else if (strcmp(token, "AUDIONODE") == 0) {
      if (parseNodeInfo && newNode) {
        audioCluster.push_back(newNode);
      }

      parseNodeInfo = true;
      newNode = new audioNode;
    }
    else if (strcmp(token, "IP") == 0) {
      if(newNode) {
        getToken(fp, newNode->ip);
        newNode->sampleFmt = sampleFmt;
        newNode->samplingRate = samplingRate;
        newNode->channels = channels;
        newNode->framePerBuffer = framePerBuffer;
        newNode->deviceId = deviceId;
      }
    }
    else if (strcmp(token, "ATTACH") == 0) {
      if(newNode) {
        getToken(fp, token);
        int nodeNum = atoi(token);

        for (int i=0; i<nodeNum; i++) {
          attachedIP = (char*) malloc(ip_length * sizeof(char));
          getToken(fp, attachedIP);
          newNode->streamIPs.push_back(attachedIP);
          SAGE_PRINTLOG("------------> attached node : %d", attachedIP);
        }

      }
    }
    else if (strcmp(token, "AUDIODIR") == 0) {
      getToken(fp, audioDir);
      audioServer = true;
    }
  }


  if (parseNodeInfo && newNode) {
    audioCluster.push_back(newNode);
  }

  if (fileContinue)
    return 1;
  else
    return 0;
}
