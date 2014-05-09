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

#ifndef _QUANTAPLUS_CONDITION_C
#define _QUANTAPLUS_CONDITION_C

#include <QUANTA/QUANTAts_mutex_c.hxx>

#ifdef QUANTA_USE_PTHREADS
typedef pthread_cond_t QUANTA_COND_T;
#elif defined(QUANTA_USE_WINDOWSTHREADS)
class _QUANTA_COND_T
{
public:
  _QUANTA_COND_T() {
    waiters = 0;
    wasBroadcast = 0;
    sema = CreateSemaphore(NULL, 0, 0xFFFFFFFF, NULL);
    waitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    waitersLock = CreateMutex(NULL, FALSE, NULL);
  }

  virtual ~_QUANTA_COND_T() {
    if (waitersDone) CloseHandle(waitersDone);
    if (waitersLock) CloseHandle(waitersLock);
    if (sema) CloseHandle(sema);
  }

  long waiters;  // Number of waiting threads.
  HANDLE waitersLock; // Serialize access to the waiters count.

  // Queue up threads waiting for the condition to become signaled.
  HANDLE sema;

  // An auto reset event used by the broadcast/signal thread to wait
  // for the waiting thread(s) to wake up and get a chance at the
  // semaphore.
  HANDLE waitersDone;

  // Keeps track of whether we were broadcasting or just signaling.
  size_t wasBroadcast;
};

typedef _QUANTA_COND_T * QUANTA_COND_T;
#else
#error One of QUANTA_USE_PTHREADS or QUANTA_USE_WINDOWSTHREADS must be defined.
#endif

/**
   Class for thread condition variables.
   Condition variables are used in conjunction with
   mutexes to provide a way for threads to wait until
   a condition occurs.

   An example of waiting on a signal is:

   \begin{verbatim}

   // Lock your mutex that is protecting someState.
   myMutex->lock();

   // Watch for your desired state to occur.
   while(someState != reached) {

   // Wait for a signal.
   myCondition->wait(myMutex);

   .... got the condition and the lock so now continue ....

   }

   myMutex->unlock();

   \end{verbatim}

   An example of sending the signal is:

   \begin{verbatim}

   // Lock your mutex that is protecting someState.
   myMutex->lock();

   // Signal that the state has been reached.
   if (someState == reached) myCondition->signal();

   // Unlock your mutex so that the waiting thread can continue.
   myMutex->unlock();

   \end{verbatim}
*/

class QUANTAts_condition_c {
public:

  /** Wait on a condition to be signalled.
      This function first releases the mutex and then
      waits on the condition. When the condition
      arises (ie it has been signaled) the mutex is
      reaquired, and the function returns.
      @return 0 if function successfully completes else non-zero
  */
  int wait(QUANTAts_mutex_c *mutex);

  /** Signal that a condition has arisen. This wakes up
      one thread that is suspended on this condition. If
      no threads are suspended this call has no effect.

      @return 0 if function successfully completes else non-zero


  */
  int signal();

  /** Signal that a condition has arisen. This wakes up
      ALL threads that are suspended on this condition. If
      no threads are suspended this call has no effect.

      @return 0 if function successfully completes else non-zero
  */
  int broadcastSignal();

  /// Return condition variable.
  QUANTA_COND_T *getCondition();

  QUANTAts_condition_c();
  ~QUANTAts_condition_c();

private:
  QUANTA_COND_T itsCV;
};


#endif
