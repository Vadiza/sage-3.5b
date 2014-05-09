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

#ifndef _QUANTAPLUS_SOCKETBASE_C
#define _QUANTAPLUS_SOCKETBASE_C

#ifndef _WIN32_WCE
#include <fcntl.h>
#endif

#ifdef WIN32
#ifndef __HAVE_WINSOCK2_H
#include <winsock2.h>
#define __HAVE_WINSOCK2_H
#endif /* __HAVE_WINSOCK2_H */
#else
#ifndef __HAVE_ARPA_INET_H
#include <arpa/inet.h>
#define __HAVE_ARPA_INET_H
#endif
#ifndef __HAVE_NETINET_IN_H
#include <netinet/in.h>
#define __HAVE_ARPA_INET_H
#endif
#endif /* WIN32 */

#include "QUANTAglobals.hxx"

//These are the number of packets for which all calculations are done -
//this is to be changed as desired

#define LATENCY_BUF_SIZE 100
#define BANDWIDTH_BUF_SIZE 100

/** A base level socket class subclassed by QUANTAnet_tcp_c, QUANTAnet_udp_c, QUANTAnet_mcast_c, QUANTAnet_parallelTcp_c,  and QUANTAnet_perfMonitor_c. The class itself does no networking. It provides some common member functions that are useful for network coding. In general one does not create an instance of this type.

    This class also offers information about the bandwidth, latency, Inter-Message_delay, Jitter and Burstiness values associated with the individual socket connections. Latency and Jitter calculations are offered only if the individual subclasses have implemented and enabled it. Otherwise values will be zero.
    These calculations are used by the QUANTAnet_perfMonitor_c class
*/

class QUANTAnet_socketbase_c {
public:

  ///Socketbase class return values
  //@{
  /// Operation went ok
  static const int OK/* = 1*/;
  /// Operation failed
  static const int FAILED/* = 0*/;
  /// Operation timed out
  static const int TIMED_OUT/* = 2 */;
  //@}

  QUANTAnet_socketbase_c();
  virtual ~QUANTAnet_socketbase_c() {}

  /// Convert a hostname to an ip address of xxx.xxx.xxx.xxx
  static int hostnameToIP(const char* hostname, char* hostIP);

  /// Convert ip address to a string of xxx.xxx.xxx.xxx
  static void ipNumToString(unsigned int ip, char* ipstring);

  /// Get time in secs with microsecond accuracy
  static double getTimeInSecs();

  /// Get the full host name of a local client
  static void getHostName(char* name, int len);

  /// If hostname is not fully expanded (ie does not include its domain) then expand it.
  static void expandHostName(const char* src, char* newname, int len);

  /// Get the host IP address as a long integer.
  static long getSelfIP();

  /// Get the host IP address as a character string.
  static void getSelfIP(char*);

  /// Get the total data sent by a connection.
  unsigned int getTotalDataSent();

  /// Get total data received by a connection.
  unsigned int getTotalDataRead();

  /// Get total instantaneous incoming bandwidth by a connection.
  double getInstantReceiveBandwidth();

  /// Get total instantaneous outgoing bandwidth by a connection.
  double getInstantSendBandwidth();

  /// Get average incoming bandwidth by a connection.
  double getAverageReceiveBandwidth();

  /// Get average outgoing bandwidth by a connection.
  double getAverageSendBandwidth();

  ///Set the instantaneous latency and also calculate the STALatency and Jitter values automatically
  void setInstantLatency(double lat);

  /// Retrieve most recent incoming packet's latency.
  double getInstantLatency();

  /// Retrieve average incoming latency.
  double getAverageLatency();

  /// Retrieve max latency so far.
  double getMaxLatency();

  /// Retrieve min latency so far.
  double getMinLatency();

  /// Record amount of data sent.
  void incrementDataSent(unsigned long size);

  /// Record amount of data read - and also compute instantaneous, average bandwidths along with the inter message delay values
  void incrementDataRead(unsigned long size);

  /// Get instantaneous inter-message delay (jitter)
  double getInstantInterMesgDelay();

  /// Get max inter-message delay (jitter)
  double getMaxInterMesgDelay();

  /// Get min inter-message delay (jitter)
  double getMinInterMesgDelay();

  /// Get average inter-message delay (jitter)
  double getAverageInterMesgDelay();

  ///Get the Short Term Average Bandwidth -over a fixed number of packets (defined by BANDWIDTH_BUF_SIZE)
  double getSTABandwidth();

  ///Get the Short Term Average Latency -over a number of packets (defined by LATENCY_BUF_SIZE)
  double getSTALatency();

  ///get the burstinesss
  double getBurstiness();

  ///Get the Jitter
  double getJitter();

  ///Return the total number of packets read
  unsigned long getPacketsRead();

  ///Return the total number of packets sent
  unsigned long getPacketsSent();

  ///Returns the socket id
  virtual int getSocketId();

  ///Returns the status of the sockets being polled
  static int selectSock(void *ptr, int *status, int numSocks,
                        struct timeval *timeout);


protected:
  double instLat, latTotal, minLat, maxLat, avgLat;
  unsigned long latCount,sentCount, readCount;
  double instDelay, totalDelay, minDelay, maxDelay, avgDelay, prevLocalTime;
  double currentTime, previousSentTime, previousReadTime,initialSentTime, initialReadTime;
  unsigned long prevTotalDataRead, prevTotalDataSent, totalDataRead, totalDataSent;
  double instReadBandwidth, instSendBandwidth, avgReadBandwidth, avgSendBandwidth;

  //STA - Short Term Average -calculated for  a specified number of packets (calcualted over a number of packets defined by LATENCY_BUF_SIZE)

  //Jitter is the average of the deviations of the instantaneous latencies from the STALatency

  //Burstiness(for bandwidths) is calculated the same way as Jitter is calculated for latencies

  double STALatency, STABandwidth, jitter, burstiness;

  //The latencies and bandwidth are stored in a buffer of defined size and the short term sums -(STlatencySum and STBandwidthSum) are calculated
  double latencyBuffer[LATENCY_BUF_SIZE];
  double bandwidthBuffer[BANDWIDTH_BUF_SIZE];
  double STlatencySum, STbandwidthSum;

  //to simplify Short Term Average calculations
  int tempLatCounter, tempBWCounter;

  ///Calculate the instantaneous outgoing bandwidth
  void computeInstantSendBandwidth();

  ///Calculate the instantaneous incoming bandwidth
  void computeInstantReceiveBandwidth();

  ///Calculate the average incoming bandwidth
  void computeAverageReceiveBandwidth();

  ///Calculate the average outgoing bandwidth
  void computeAverageSendBandwidth();

  ///Calculate the jitter over a specified number of packets
  void calculateJitter(double lat);

  ///Calculate the burstiness over a specified number of packets
  void calculateBurstiness(double bw);


};


#endif
