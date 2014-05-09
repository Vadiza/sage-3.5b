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

#ifndef _QUANTATS_RWLOCK_C_H
#define _QUANTATS_RWLOCK_C_H

#include <QUANTA/QUANTAconfig.h>

#ifdef QUANTA_USE_PTHREADS
#  ifndef __HAVE_PTHREAD_H
#    include <pthread.h>
#    define __HAVE_PTHREAD_H
#  endif /* __HAVE_PTHREAD_H */
typedef pthread_rwlock_t QUANTA_RWLOCK_T;
#elif defined(QUANTA_USE_WINDOWSTHREADS)
# ifndef __HAVE_WINSOCK2_H
# include <winsock2.h>
# define __HAVE_WINSOCK2_H
#endif /* __HAVE_WINSOCK2_H */
typedef HANDLE QUANTA_RWLOCK_T;
#else
#  error One of QUANTA_USE_PTHREADS or QUANTA_USE_WINDOWSTHREADS must be defined.
#endif

/** Read and write locking class.

    This class encapsulates a read-write lock in an object. Any number
    of threads can hold a given read-write lock for reading as long as
    no thread holds the read-write lock for writing. A read-write lock
    can be allocated for writing only if no thread holds teh
    read-write lock for reading or writing. Read locks are considered
    shared among threads, while write locks are exclusive.
*/
class QUANTAts_rwlock_c
{
public:

  /// Creates a QUANTA read-write lock.
  QUANTAts_rwlock_c();

  /// Destroys a QUANTA read-write lock.
  ~QUANTAts_rwlock_c();

  /// @name Obtaining the lock
  //@{
  /** Lock the object for reading.
      @return 0 on success, positive value otherwise
  */
  int readLock();

  /** Lock the object for both reading and writing.
      @return 0 on success, positive value otherwise
  */
  int writeLock();
  //@}

  /// @name Trying to access the lock
  //@{
  /** Try to obtain a write lock (returns immediately).
      @return 0 if obtained the lock, positive value otherwise
  */
  int tryReadLock();

  /** Try to obtain a write lock (returns immediately).
      @return 0 if obtained the lock, positive value otherwise
  */
  int tryWriteLock();
  //@}

  /** Unlock the object for both reading and writing.
      @return 0 on success, positive value otherwise
  */
  int unlock();

  /** Turn debug messages on or off.
      @param debugStatus show debug messages, 0 = no, 1 = yes
  */
  void showDebug(const short& debugStatus);

  /** Set the debug message to print whenever lock and unlock are performed.
      @param msg string prepended to debug messages
  */
  void setDebugMessage(const char* msg);

  /// Return the handle to the mutex variable.
  QUANTA_RWLOCK_T* getRWLock();

private:
  /// Read-write lock primitive.
  QUANTA_RWLOCK_T rwlock;

  /// String displayed with debug messages.
  char debugMessage[1024];

  /// Indicates whether to display debug messages or not.
  short debugStatusOn;
};

#endif /* _QUANTATS_RWLOCK_C_H */
