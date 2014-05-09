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

/*
  QUANTAnet_udpReflector_c
  - setIncomingPort(int)
  - setTimeOutTime(int)
  - setForcedDestination(ip,port);
  - newDataCB()
  - deliver(data)
  - setMulticast(ON or OFF);
  - setMulticastPort(int)
  - setMulticastAddress()

  QUANTAnet_tcpReflector_c
  - setIncomingPort(int)
  - setTimeOutTime(int)
  - setForcedDestination(ip,port);
  - newDataCB()
  - deliver(data)
*/

#ifndef QUANTANET_UDP_REFLECTOR
#define QUANTANET_UDP_REFLECTOR

#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_mcast_c.hxx>
#include <QUANTA/QUANTAts_thread_c.hxx>
#include <QUANTA/QUANTAts_mutex_c.hxx>

#define QUANTANET_UDP_REFLECTOR_MAX_MSG_SIZE 8192
#define QUANTANET_UDP_REFLECTOR_SEND_TO_ALL_UDP_CLIENTS -1
#define QUANTANET_UDP_REFLECTOR_MAX_NUM_CLIENTS 1024
#define QUANTANET_UDP_REFLECTOR_DEFAULT_TIMEOUT 60
#define QUANTANET_UDP_REFLECTOR_IP_SIZE 256
#define QUANTANET_UDP_REFLECTOR_MCAST_TTL 160
#define PERFDAEMON_ALIVE 1
#define PERFDAEMON_NOT_ALIVE 0

/** UDP and Multicast Reflector class.
    Even though QUANTA's key, link, and channel concept essentially acts as a data
    reflector there are  instances where a generic packet reflector (to support legacy
    applications) are needed. This class fulfills that need.

    This class will accept incoming UDP packets and broadcast
    them to all listeners. This class also accepts incoming multicast packets and broadcasts them to
    all listeners (whether they are on UDP or Multicast). In that case
    all incoming UDP packets will also be broadcasted to the Multicast group.

    To add a listener to the reflector the remote client simply needs to send a packet to
    the reflector. The reflector will use the packet's address as the return address
    for future data. If a client does not send any data for a certain period  of time
    that client is removed from the reflector's list of listeners.

*/
class QUANTAnet_udpReflector_c : public QUANTAnet_socketbase_c
{

  // JASON
private:
  QUANTAts_thread_c *udpThread, *mcastThread;
  QUANTAts_mutex_c *iomutex;
  // JASON- not intended to be public
  static void * threadUDP(void * inData);
  static void * threadMCAST(void * inData);

public:
  QUANTAnet_udpReflector_c();
  virtual ~QUANTAnet_udpReflector_c();


  //@{
  /// Status Failed
  static const int FAILED/* = 0*/;

  /// Status Ok.
  static const int OK/* = 1*/;

  /// Skip the data distribution process. Used in user callback. See intercept().
  static const int SKIP_DISTRIBUTION/* = 2*/;

  //@}

  /** Initialize the reflector once all the parameters are set up.
      If you need to change the settings at a later time you need to delete
      this object and create a new one with the new settings.
      @return FAILED if failed; OK if success.
  */
  int init();

  /** Set the incoming port for the reflector to listen on. Default is set at 6000 */
  void setIncomingPort(unsigned short port = 6000);

  /** Set the time out time when connected clients who have not sent any data for
      a while will be removed. Default is set at 60 seconds.
  */
  void setTimeOutTime(int secs);

  /** Set the number of seconds to wait for blocking operations to timeout.
      @param seconds
      Number of seconds to wait before timing out a blocking socket call.
  */
  void setBlockingTimeout(const int& seconds);

  /// Set a single ip address and port number to which all packets will be sent.
  void setForcedDestination(char *ipAddr, unsigned short port);

  /** Set up to also listen on a multicast address and port.
      This means that any incoming packets on the UDP connection will get broadcasted
      via this reflector to the multicast channel too.
  */
  void setMulticastAddress(char *ipAddr,unsigned short port);

  /** Set multicast TTL to something other than the default at 160 */
  void setMulticastTTL(int ttl);

  /** Call this within a while loop to let the reflector continuously
      do its processing.
      @return FAILED if error; OK if success.
  */
  int process();

  // JASON - not intended to be used by the user, but it cannot be made
  // private because this is called by the thread.
  int processMCAST();
  int processUDP();

  /** Intercept incoming messages and call a user-defined callback function.
      If you want you can also alter the buffer completely so that the reflector will reflect an entirely different message. You can do this by changing the contents of the buffer or  by replacing the buffer entirely by allocating memory for a new buffer and stuffing it with your own data. If you choose to allocate a totally new buffer you must remember  to deallocate memory for the original buffer before substituting it with yours.                                              If after your callback function exits you do not wish the reflector to forward  the contents of the buffer, return with QUANTAnet_udpReflector_c::SKIP_DISTRIBUTION. Otherwise just return QUANTAnet_udpReflector_c::OK.

      Note also that the callback function will also be given a pointer to a QUANTAnet_udp_c object that can then be used to send data directly to the client that originally sent the message.
  */
  void intercept(int (*callback) (QUANTAnet_udp_c *client, char** buffer, int *bufferSize,  void *userData), void* userData);

  /**Enable instrumentation - this is for turning on the performance monitoring for the UDPsocket used -instrumentation for the udp sockets are turned off by default - and it should be explicitly enabled for latency, jitter and related statistics
   */
  void enableInstrumentation();

  /**Disable instrumentation - this is for turning off the performance monitoring for the UDPsocket used*/

  void disableInstrumentation();

  //Functions added for performance monitoring interface

  /**
     Displays the resultant statistics instantaneously in the netlogger format - this should be typically done after a read/write is done  a network.

     Also, it should be noted that a showStats call should be made at the end of atleast one send and receive for two-way information (the same applies for logStats and sendStats)

     @param streamInfo
     A label describing the stream that is being monitored.

     @param comment
     A comment on the event that marks the time at which the stream is being monitored
  */
  void showStats(char* streamInfo, char* comment);

  /**
     This logs performance statistics in a file. The user opens a file and passes the file pointer with this function and results of monitoring are written into the logfile.

     @param streamInfo
     A label describing the stream that is being monitored.

     @param comment
     A comment on the event that marks the time at which the stream is being monitored

     @param filePtr
     File pointer to the file in which the results of monitoring are to be stored

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int logStats(char* streamInfo, char* comment, FILE* filePtr);

  /**
     Sends the performance statistics to a remote perfdaemon -for further analysis of the monitored data - the initSendStats API should be called first, before calling a sendStats (In order to connect to the perfdaemon initially)


     @param streamInfo
     A label describing the stream that is being monitored.

     @param comment
     A comment on the event that marks the time at which the stream is being monitored

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED

  */

  int sendStats(char* streamInfo, char* comment);

  /**
     Initialize sendStats - provide the IP of the perfDaemon and an optional port number to connect to. This should be done initially  before using the sendStats API.

     @param monitorClientIP
     IP address of the perfDameon to connect to

     @param port
     Port number at which the perfDaemon is running -this is optional. The default port number for perfDaemon is 9500 -so a different port number has to be specified if the perfDaemon is running ona different port.

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int initSendStats(char* monitorClientIP, int port = PERF_DAEMON_DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */

  void exitSendStats();

private:
  double *itsAliveClients;
  QUANTAnet_udp_c *itsForcedUDP, *itsMainUDPSocket;
  QUANTAnet_mcast_c *itsMcastSocket;
  unsigned short itsUDPPort;
  int itsTimeOutTime;
  int itsMcastTTL;
  double itsStartTime;
  char itsForcedUDPIP[QUANTANET_UDP_REFLECTOR_IP_SIZE];
  char itsMcastIP[QUANTANET_UDP_REFLECTOR_IP_SIZE];
  unsigned short itsMcastPort;
  unsigned short itsForcedUDPPort;
  QUANTAnet_udp_c **itsBunchOfClients;
  int itsNumberOfClients;
  int doMcast, doForcedUDP;
  double itsLastTime;

  // JASON
  char* inBufferUDP;
  char* inBufferMCAST;

  int itsInit;
  int initClientList();
  int addClient(QUANTAnet_udp_c*);
  void replyToClients(char *mesg, int size, int exceptClientIndex);
  void purgeOldClients();
  int  checkClient(QUANTAnet_udp_c *mainUDPSocket);

  int (*interceptCallback) (QUANTAnet_udp_c *newClient, char** buffer, int* bufsize, void *userData);
  void *interceptUserData;

  char* perfDaemonIP;     //IP of the perf-daemon - used for initSendStats
  int perfDaemonPort;     //PerfDaemon port
  int perfDaemonStatus;   //Status that indicates if the perfdaemon is running

};
#endif
