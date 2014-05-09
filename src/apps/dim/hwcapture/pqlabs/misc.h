/*   OmegaTable Tracker                 */
/*   Copyright (C) 2008-2009 Khairi Reda, Hyejung Hur, Edward Kahler,       */
/*   Dennis Chau, Jason Leigh, Andrew Johnson, Luc Renambot                 */
/*   Electronic Visualization Laboratory                                    */
/*   University of Illinois at Chicago                                      */
/*   http://www.evl.uic.edu/                                                */
/*                                                                          */
/*   This is free software; you can redistribute it and/or modify it        */
/*   under the terms of the  GNU General Public License  as published by    */
/*   the  Free Software Foundation;  either version 2 of the License, or    */
/*   (at your option) any later version.                                    */
/*                                                                          */
/*   This program is distributed in the hope that it will be useful, but    */
/*   WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of    */
/*   MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU    */
/*   General Public License for more details.                               */

/* ******************************************
 * Misc functions
 * misc.h
 * ******************************************
 */

#ifndef _MISC_H_
#define _MISC_H_


#include <CoreServices/CoreServices.h>


typedef long long TIMESTAMP;
typedef uint64_t U64;

double getTime();
TIMESTAMP getIntTime();

double round(double x);
float round(float x);

#endif
