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

#ifndef _QUANTAPLUS_QUANTAPLUS_MCAST_CLASS
#define _QUANTAPLUS_QUANTAPLUS_MCAST_CLASS

#include <QUANTA/QUANTAnet_socketbase_c.hxx>

#ifdef WIN32
#include <Ws2tcpip.h>
#endif /* WIN32 */

/** Multicast Class. This class bypasses QUANTA's standard method for establishing connections and provides the user with direct control of multicast connections.
 */

class QUANTAnet_mcast_c : public QUANTAnet_socketbase_c {

public:

  //@{
  /// Status ok.
  static const int OK/* = 1*/;

  /// Status failed.
  static const int FAILED/* = 0*/;
  //@}

  static const int maxTTL;

  QUANTAnet_mcast_c();

  /// Close the connection
  void close();

  /// Open a connection to the multicast IP address
  int init(char *mcastGroup, int port, int mttl = QUANTAnet_mcast_c::maxTTL);

  /// Broadcast a message to the group
  int send(char *, int);

  /// Receive a message from the group.
  int receive(char *, int);

  /// Print info on the current mcast connection.
  void printInfo();

  /// Tell mcast to use non-blocking data transfer.
  void makeNonBlocking();

  /// Get the IP address of the last message received
  unsigned long getReceiveIP();

  /// Same thing except it returns a string */
  void getReceiveIP(char *clientIP);

  /* Get the IP address of the mcast group. This shouldn't
   * be anything you'd forget since you needed a number in the 1st
   * place to start, but it's provided here for completeness.
   */

  /// Accessor Functions
  //@{
  /// Get outgoing packet's destination address.
  unsigned long getSendIP();
  /// Get outgoing packet's destination address.
  void getSendIP(char *ip);
  /// Get your own port.
  unsigned short getSelfPort();
  /// Get outgoing port.
  unsigned short getSendPort();
  /// Get incoming port.
  unsigned short getReceivePort();
  //@}

  /** Enable the logging of latency and inter-message delay.
      Note: in order for this to work, all clients that subscribe to
      the same multicast address must also enable their instrumentation or else
      the bit stream will become confused. Enabling logging prepends timestamp
      data that is used to compute latency. By default instrumentation
      is disabled except for Bandwidth measurements.
  */
  void enableInstrumentation();

  /** Disable the logging of latency and inter-message delay.
      Note: in order for this to work, all clients that subscribe to
      the same multicast address must also disable their instrumentation or else
      the bit stream will become confused. By default instrumentation
      is disabled except for Bandwith measurements.

  */
  void disableInstrumentation();

private:

  int enableInstr;
  int sockfd;
  struct sockaddr_in receiveAddress, sendAddress;
  struct ip_mreq mreq;
  char ttl;

};
#endif
