/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageMessage.cpp - interface functions of the sageMessage class
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

#include "sageBase.h"

sageMessage::sageMessage() : clientID(0)
{
  dest = NULL;
  code = NULL;
  appCode = NULL;
  size = NULL;
  data = NULL;
  buffer = NULL;
}

sageMessage::~sageMessage()
{
}

int sageMessage::destroy()
{
  if (buffer)
    delete [] buffer;

  return 0;
}

/*
  void sageMessage::init()
  {
  int fSize = MESSAGE_FIELD_SIZE;

  size = buffer;
  dest = &buffer[fSize];
  code = &buffer[2*fSize];
  appCode = &buffer[3*fSize];
  data = (void *)&buffer[4*fSize];
  }
*/

int sageMessage::init(int buf_size)
{
  buffer = new char[buf_size+1];
  memset(buffer, 0, buf_size+1);
  int fSize = MESSAGE_FIELD_SIZE;
  size = buffer;
  dest = &buffer[fSize];
  code = &buffer[2*fSize];
  appCode = &buffer[3*fSize];
  if (buf_size > MESSAGE_HEADER_SIZE)
    data = (void *)&buffer[4*fSize];
  bufSize = buf_size;
  sprintf(size,"%d\0", bufSize);

  return buf_size;
}

int sageMessage::init(int dst, int co, int app,
                      int sz, const void *dat)
{
  if ((bufSize = init(sz + MESSAGE_HEADER_SIZE)) < 0)
    return -1;

  sprintf(dest,"%d\0", dst);
  sprintf(code,"%d\0", co);
  sprintf(appCode,"%d\0", app);

  if (dat)
    memcpy(data, dat, sz);

  return bufSize;
}

int sageMessage::set(int dst, int co, int app,
                     int sz, const void *dat)
{
  sprintf(dest,"%d\0", dst);
  sprintf(code,"%d\0", co);
  sprintf(appCode,"%d\0", app);
  sprintf(size,"%d\0", sz + MESSAGE_HEADER_SIZE);
  memcpy(data, dat, sz);

  return 0;
}

int sageMessage::getDest()
{
  if (dest) {
    dest[8] = '\0';
    return atoi(dest);
  }
  else {
    std::cerr << "fail to get destination" << std::endl;
    return 0;
  }
}

int sageMessage::getCode()
{
  if (code) {
    code[8] = '\0';
    return atoi(code);
  }
  else {
    //std::cerr << "fail to get code" << std::endl;
    return 0;
  }
}

int sageMessage::getAppCode()
{
  if (appCode) {
    appCode[8] = '\0';
    return atoi(appCode);
  }
  else {
    std::cerr << "fail to get application code" << std::endl;
    return 0;
  }
}

int sageMessage::getSize()
{
  if (size) {
    size[8] = '\0';
    return (atoi(size) - MESSAGE_HEADER_SIZE);
  }
  else {
    std::cerr << "fail to get size of message" << std::endl;
    return 0;
  }
}

void* sageMessage::getData()
{
  if (data)
    return data;
  else {
    return NULL;
  }
}

char* sageMessage::getBuffer()
{
  return buffer;
}

int sageMessage::setSize(int s)
{
  if (s > bufSize - MESSAGE_HEADER_SIZE) {
    std::cout << "sage message buffer overflow" << std::endl;
    return -1;
  }

  bufSize = s + MESSAGE_HEADER_SIZE;
  sprintf(size,"%d\0", bufSize);
  return 0;
}

int sageMessage::setDest(int dst)
{
  if (dest)
    sprintf(dest,"%d\0", dst);
  else {
    std::cerr << "fail to set destination" << std::endl;
    return -1;
  }
  return 0;
}

int sageMessage::setCode(int co)
{
  if (code)
    sprintf(code,"%d\0", co);
  else {
    std::cerr << "fail to set code" << std::endl;
    return -1;
  }
  return 0;
}

int sageMessage::setAppCode(int app)
{
  if (appCode)
    sprintf(appCode,"%d\0", app);
  else {
    std::cerr << "fail to set application code" << std::endl;
    return -1;
  }
  return 0;
}

int sageMessage::setData(int sz, void* dat)
{
  if (sz > bufSize - MESSAGE_HEADER_SIZE) {
    std::cout << "sage message buffer overflow" << std::endl;
    return -1;
  }

  bufSize = sz + MESSAGE_HEADER_SIZE;

  if (size && data) {
    sprintf(size,"%d\0", bufSize);
    memcpy(data, dat, sz);
  }
  else {
    std::cerr << "fail to set data" << std::endl;
    return -1;
  }
  return 0;
}
