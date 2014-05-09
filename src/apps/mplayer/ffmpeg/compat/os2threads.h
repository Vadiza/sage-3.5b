/*
 * Copyright (c) 2011 KO Myung-Hun <komh@chollian.net>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * os2threads to pthreads wrapper
 */

#ifndef AVCODEC_OS2PTHREADS_H
#define AVCODEC_OS2PTHREADS_H

#define INCL_DOS
#include <os2.h>

#undef __STRICT_ANSI__          /* for _beginthread() */
#include <stdlib.h>

typedef TID  pthread_t;
typedef void pthread_attr_t;

typedef HMTX pthread_mutex_t;
typedef void pthread_mutexattr_t;

typedef struct {
  HEV  event_sem;
  int  wait_count;
} pthread_cond_t;

typedef void pthread_condattr_t;

struct thread_arg {
  void *(*start_routine)(void *);
  void *arg;
};

static void thread_entry(void *arg)
{
  struct thread_arg *thread_arg = arg;

  thread_arg->start_routine(thread_arg->arg);

  av_free(thread_arg);
}

static av_always_inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg)
{
  struct thread_arg *thread_arg;

  thread_arg = av_mallocz(sizeof(struct thread_arg));

  thread_arg->start_routine = start_routine;
  thread_arg->arg = arg;

  *thread = _beginthread(thread_entry, NULL, 256 * 1024, thread_arg);

  return 0;
}

static av_always_inline int pthread_join(pthread_t thread, void **value_ptr)
{
  DosWaitThread((PTID)&thread, DCWW_WAIT);

  return 0;
}

static av_always_inline int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
  DosCreateMutexSem(NULL, (PHMTX)mutex, 0, FALSE);

  return 0;
}

static av_always_inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  DosCloseMutexSem(*(PHMTX)mutex);

  return 0;
}

static av_always_inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  DosRequestMutexSem(*(PHMTX)mutex, SEM_INDEFINITE_WAIT);

  return 0;
}

static av_always_inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  DosReleaseMutexSem(*(PHMTX)mutex);

  return 0;
}

static av_always_inline int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  DosCreateEventSem(NULL, &cond->event_sem, DCE_POSTONE, FALSE);

  cond->wait_count = 0;

  return 0;
}

static av_always_inline int pthread_cond_destroy(pthread_cond_t *cond)
{
  DosCloseEventSem(cond->event_sem);

  return 0;
}

static av_always_inline int pthread_cond_signal(pthread_cond_t *cond)
{
  if (cond->wait_count > 0) {
    DosPostEventSem(cond->event_sem);

    cond->wait_count--;
  }

  return 0;
}

static av_always_inline int pthread_cond_broadcast(pthread_cond_t *cond)
{
  while (cond->wait_count > 0) {
    DosPostEventSem(cond->event_sem);

    cond->wait_count--;
  }

  return 0;
}

static av_always_inline int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  cond->wait_count++;

  pthread_mutex_unlock(mutex);

  DosWaitEventSem(cond->event_sem, SEM_INDEFINITE_WAIT);

  pthread_mutex_lock(mutex);

  return 0;
}

#endif /* AVCODEC_OS2PTHREADS_H */
