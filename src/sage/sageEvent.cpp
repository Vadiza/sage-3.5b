/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageEvent.cpp
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
 * Direct questions, comments etc about SAGE to sage_users@listserv.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#include "sageEvent.h"

sageEvent::sageEvent(int type, char *msg, void *p)
{
  eventType = type;
  setMsg(msg);
  param = p;
  buflen = 0;
}


sageSyncEvent::sageSyncEvent(int type, int bl, void *p) {
  eventType = type;
  param = p;
  buflen = bl;

  if ( bl > 0 ) {
    eventMsg = (char *)malloc(sizeof(char) * bl);
  }
}

int sageSyncEvent::setMsg(char *msg) {
  if (eventMsg && msg) {
    if ( ! strcpy(eventMsg, msg) ) return -1;
    buflen = strlen(eventMsg);

    if ( buflen <= 0 ) return -1;

    return buflen;
  }
  return -1;
}

sageEventQueue::sageEventQueue() : empty(true)
{
  eventQueue.clear();

  queueLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(queueLock, NULL);
  pthread_mutex_unlock(queueLock);
  notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(notEmpty, NULL);
}

sageEvent* sageEventQueue::getEvent()
{
  pthread_mutex_lock(queueLock);

  while (isEmpty()) {
    //std::cerr << "=== waiting for new events === " << std::endl;
    pthread_cond_wait(notEmpty, queueLock);
  }

  sageEvent *event = eventQueue.front();
  eventQueue.pop_front();

  pthread_mutex_unlock(queueLock);

  return event;
}

void sageEventQueue::sendEvent(sageEvent* event)
{
  pthread_mutex_lock(queueLock);
  eventQueue.push_back(event);
  pthread_mutex_unlock(queueLock);
  pthread_cond_signal(notEmpty);
}

void sageEventQueue::sendEventToFront(sageEvent* event)
{

  pthread_mutex_lock(queueLock);
  eventQueue.push_front(event);
  pthread_mutex_unlock(queueLock);
  pthread_cond_signal(notEmpty);

}

void sageEventQueue::sendEvent(int type, char *msg, void *p)
{
  sageEvent *event = new sageEvent(type, msg, p);
  sendEvent(event);
}

void sageEventQueue::sendEvent(int type, int info, void *p)
{
  sageEvent *event = new sageEvent(type, NULL, p);
  sprintf(event->eventMsg, "%d", info);
  sendEvent(event);
}

sageEventQueue::~sageEventQueue()
{
  if (queueLock)
    free(queueLock);

  if (notEmpty)
    free(notEmpty);

  //eventQueue.clear();
}
