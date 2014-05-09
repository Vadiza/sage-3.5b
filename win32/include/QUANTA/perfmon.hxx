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

/*This file has functions that are used for performance monitoring while using udp connections with the QUANTAnet_udp_c class
  A header containing the timestamp is prepended while a message is sent and the header is unprepended when the message is read.

  Note:
  Instrumentation should be enabled on both the server and the client sides to turn on the performance monitoring
*/

#ifndef _QUANTA_PERFMON
#define _QUANTA_PERFMON

///return the size of the performance_monitoring header
int _quanta_getSizeOfPerformanceMonitoringHeader();

///Prepend the message with the performance_monitoring header
void _quanta_prependPerformanceMonitoringHeader(char* newmsg, char* message, int length);

///Allocate memory for the new message - to include the performance_monitoring header
char* _quanta_allocMemForPerformanceMonitoredMesg(int length, int * newlength);

///Unprepend the performance_monitoring header
void _quanta_unprependPerformanceMonitoringHeader(double *remoteTimeStamp, double *inlatency, char* newmsg, int newlen,  char* message, int *ulen);


#endif
