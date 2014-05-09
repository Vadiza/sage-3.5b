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

/******************************************************************
 * QUANTA Barrier Class
 *
 *
 * NOTE: most of this code is adapted from "Thread Time
 * The Multithreaded Programming Guide" ISBN 0-13-190067-6
 ******************************************************************/


#ifndef _QUANTAPLUS_BARRIER_C
#define _QUANTAPLUS_BARRIER_C

#include <QUANTA/QUANTAts_mutex_c.hxx>
#include <QUANTA/QUANTAts_condition_c.hxx>

/**
   class type: threads sync. primative
   used for: block continued execution until N threads reach the barrier
*/
class QUANTAts_barrier_c
{
public:
  /**
     creates a new barrier that will block for the number of threads specified
     in the parameter num_threads

     @param number of threads to wait for
  */
  //    QUANTAts_barrier_c(void) { QUANTAts_barrier_c(1); }
  QUANTAts_barrier_c(int num_threads);
  ~QUANTAts_barrier_c(void); /** destroy this barrier */


  /** wait() blocks continued execution until all N threads reach this barrier

      @return 0 if function successfully completes else non-zero
  */
  int wait(void);


private:
  QUANTAts_mutex_c _mutex; /* mutex variable */
  QUANTAts_condition_c _cv; /* condition variable */
  int _barrier_val; /* # of threads to wait */
  int _blocked_threads; /* # of threads waiting */
  int _predicate; /* condition predicate */

};

#endif
