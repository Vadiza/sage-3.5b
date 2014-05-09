/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#if defined(WIN32) && defined(QUANTA_USE_WINDOWSTHREADS)

#ifndef __PTHREAD_WIN_H_
#define __PTHREAD_WIN_H_

/* This program is largely based on "Strategies for Implementing POSIX Condition
   Variables on Win32" written by Douglas C. Schmidt and Irfan Pyarali and published
   in C++ Report, Vol. 10, No. 5, June, 1998 issue. You can get the full article from
   "http://www.cs.wustl.edu/~schmidt/win32-cv-1.html". I fixed a few things from the
   code and added pthread_win_cond_destroy function.
*/

#include <QUANTA/QUANTAts_mutex_c.hxx>

struct pthread_win_cond_t
{
  int waiters_count_;
  // Count of the number of waiters.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  int release_count_;
  // Number of threads to release via a <pthread_cond_broadcast> or a
  // <pthread_cond_signal>.

  int wait_generation_count_;
  // Keeps track of the current "generation" so that we don't allow
  // one thread to steal all the "releases" from the broadcast.

  HANDLE event_;
  // A manual-reset event that's used to block and release waiting
  // threads.
};

int pthread_win_cond_init(struct pthread_win_cond_t *cv);
int pthread_win_cond_wait(struct pthread_win_cond_t *cv,
                          QUANTAts_mutex_c *external_mutex);
int pthread_win_cond_signal(struct pthread_win_cond_t *cv);
int pthread_win_cond_broadcast(struct pthread_win_cond_t *cv);
int pthread_win_cond_destroy(struct pthread_win_cond_t *cv);

#endif

#endif
