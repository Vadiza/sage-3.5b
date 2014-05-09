/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#ifndef _QUANTA_GLOBALS_H
#define _QUANTA_GLOBALS_H

#include <ctype.h>

//#include "QUANTAconfig.h"

// add bool, true, false support for irix o32 format
#if (defined(__sgi) && defined(HAVE_BINARY_TYPE_32) && !defined(_BOOL))
typedef int bool;
#define false 0
#define true 1
#endif


// disable ms visual c++ warnings
#ifdef WIN32
// disable ms visual c++ warning - "C++ Exception Specification ignored"
#pragma warning(disable : 4290)

// disable ms visual c++ warning - "deletion of an array expression; conversion to pointer supplied"
// maybe look into this one more
#pragma warning(disable : 4154)
#endif


// add a case sensitive string compare if not available
#ifdef WIN32
# ifndef HAVE_STRCASECMP
#   ifdef HAVE_STRICMP
#     define strcasecmp(x,y) stricmp(x,y)
#   elif defined(HAVE_STRCMPI)
#       define strcasecmp(x,y) strcmpi(x,y)
#   else // no case insensitive function available
#     define QUANTA_STRCASECMP
extern int strcasecmp(const char *first, const char *second);
#   endif
# endif
#endif

/** Platform-independent sleep function.
    Different platforms and APIs have different methods for implementing
    ways to suspect program activity. This function serves as a wrapper
    which internally calls usleep() or Sleep() as needed.
    @param microseconds number of microseconds to wait.
*/
void QUANTAsleep(const unsigned int& seconds);

/** Platform-independent usleep function.
 */
void QUANTAusleep(const unsigned int& microseconds);

#if defined(linux) || defined(__APPLE__)
#define HAVE_SOCKLEN_T
#endif

// add socklen_t to systems that do not provide it (sgi..)
#if !defined(HAVE_SOCKLEN_T)
typedef int socklen_t;
#endif


#endif
