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

#ifndef QUANTAMISC_HASH
#define QUANTAMISC_HASH

// Do we have 32bit or 64bit longs?
#if LONG_64_BITS || !INT_16_BITS
typedef int QUANTAmisc_hashWord_t;
#else
typedef long QUANTAmisc_hashWord_t;
#endif
/// Give a string, convert generate a hash table value.
QUANTAmisc_hashWord_t QUANTAmisc_hash (char *key, int ksize);
#endif
