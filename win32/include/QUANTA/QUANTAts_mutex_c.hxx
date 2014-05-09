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

#ifndef _QUANTAPLUS_MUTEX_C
#define _QUANTAPLUS_MUTEX_C

#include <QUANTA/QUANTAconfig.h>

#ifdef QUANTA_USE_PTHREADS
#  ifndef __HAVE_PTHREAD_H
#    include <pthread.h>
#    define __HAVE_PTHREAD_H
#   endif
typedef pthread_mutex_t QUANTA_MUTEX_T;
#elif defined(QUANTA_USE_WINDOWSTHREADS)
# ifndef __HAVE_WINSOCK2_H
# include <winsock2.h>
# define __HAVE_WINSOCK2_H
# endif /* __HAVE_WINSOCK2_H */
typedef HANDLE QUANTA_MUTEX_T;
#else
#  error One of QUANTA_USE_PTHREADS or QUANTA_USE_WINDOWSTHREADS must be defined.
#endif

/**
   Mutual exclusion class.

   This class encapsulates mutual exclusion in a C++ object.

*/
class QUANTAts_mutex_c {
public:

  enum mutexDebug_t {
    DEBUG_ON, DEBUG_OFF
  };

  /// Construct for a QUANTA mutual exclusion object.
  QUANTAts_mutex_c();

  /// Lock the mutex object.
  void lock();

  /// Unlock mutex object.
  void unlock();

  /// Turn mutex debug messages on or off.
  void setMutexDebug(QUANTAts_mutex_c::mutexDebug_t stat);

  /// Set the debug message to print whenever lock and unlock is performed.
  void setMutexDebugMesg(char *msg);

  /// Destructor for a QUANTA mutual exclusion object.
  ~QUANTAts_mutex_c();

  /// Return the handle to the mutex variable.
  QUANTA_MUTEX_T * getMutex();

private:
  QUANTA_MUTEX_T mutex;
  mutexDebug_t debugStat;
  char *debugMsg;
};

#endif
