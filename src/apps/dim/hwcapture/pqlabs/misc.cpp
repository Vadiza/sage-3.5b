/*   OmegaTable Tracker                                                     */
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

/* ***************************************************
 * Misc functions
 * High resolution timer borrowed from SAGE 3.0 code
 * misc.cpp
 * ***************************************************
 */

#include "misc.h"



TIMESTAMP getIntTime()
{
#if defined(__APPLE__)

  // mac
  UnsignedWide wide;
  Microseconds(&wide);

  // Mux the two portions of the timestamp into a single 64-bit value,
  // then divide the result to convert microseconds to milliseconds.
  return (U64(wide.hi) << 32) | U64(wide.lo);
#endif
}


double round(double x)
{
  if(x>=0.5){return ceil(x);}else{return floor(x);}
}

float round(float x)
{
  if(x>=0.5){return ceil(x);}else{return floor(x);}
}
