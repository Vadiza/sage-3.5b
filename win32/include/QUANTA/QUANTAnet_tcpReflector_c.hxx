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

#ifndef _QUANTAPLUS_TCP_REFLECTOR
#define _QUANTAPLUS_TCP_REFLECTOR

// Forward declarations.
class QUANTAnet_tcpClient_c;
class QUANTAnet_tcpServer_c;

#include <QUANTA/QUANTAnet_perfDaemon_c.hxx> // Needed for macros.

#define TCP_REFLECTOR_DEFAULT_PORT  7000
#define PERFDAEMON_ALIVE 1
#define PERFDAEMON_NOT_ALIVE 0

class QUANTAnet_tcpReflectorClient_c;

/** TCP Reflector class. This class treats TCP streams as discrete
    packets/messages that can be reflected to multiple connected
    clients. Clients must be created using QUANTAnet_tcpReflectorClient_c
    in order to interpret the TCP stream correctly. Hence do NOT attempt to
    use QUANTAnet_tcpClient_c to connect to a TCP reflector or it will corrupt
    the TCP reflector stream.
    This reflector has 2 main functions: checkForNewClients() and process().
    checkForNewClients, as the name suggests, checks to see if any new clients
    wish to connect to the reflector. Process() does the actual work of data reflection.

    checkForNewClients is called everytime process() is called. If you want the check
    to be done more frequently you need to do it yourself. Similarly if you want
    the process() call done more frequently it is up to you to do it. I recommend threading
    those two off and setting up a mutex so that you do not do process() and
    checkForNewClients() at the same time.
*/
class QUANTAnet_tcpReflector_c
{
public:
  QUANTAnet_tcpReflector_c();
  ~QUANTAnet_tcpReflector_c();

  //@{
  /// Status ok.
  static const int OK/* = 0*/;

  /// Status failed.
  static const int FAILED/* = 1*/;

  /// Memory allocation error.
  static const int MEM_ALLOC_ERR/* = 2*/;

  /// New client has been connected.
  static const int NEW_CONNECTION_ESTABLISHED/* = 4*/;

  /// Reflector cannot handle any more connections
  static const int TOO_MANY_CLIENTS/* = 5*/;

  /// No new connection.
  static const int NO_NEW_CONNECTION/* = 6*/;

  /// A non-blocking read had no data available to read.
  static const int NON_BLOCKING_HAS_NO_DATA/* = 7*/;

  /// Skip the data distribution process. Used in user callback. See intercept().
  static const int SKIP_DISTRIBUTION/* = 8*/;

  static const int DEFAULT_PORT/* = 7000*/;
  //@}

  /** Initialize the reflector.
      @param incomingPort is listening port for incoming connections. Default is 7000.
      @param maxClients is the max number of clients the reflector will manage.
      @param forwardAddr is the IP address of a machine running another tcpreflector to whom data must be forwared to.
      @param forwardPort is the port number of tcpreflector to whom data must be forwared to.
      @return Either QUANTAnet_tcpReflector_c::OK,FAILED,MEM_ALLOC_ERR.
  */
  int init(int incomingPort=TCP_REFLECTOR_DEFAULT_PORT, int maxClients = 64,
           const char* forwardAddr = NULL, unsigned short forwardPort = 0);

  /** Call this within a while loop to let the reflector continuously
      do its processing.
      @return Either QUANTAnet_tcpReflector_c::OK,MEM_ALLOC_ERR
  */
  int process();

  /** Call this as often as you wish to check for new clients.
      Note. If you do this in a separate thread then you must set up a mutex
      so that you do not call the proces() call and this call at the same time.
      The process() call itself has embedded in it 1 check for each time you
      call it.
      @return One of QUANTAnet_tcpReflector_c::NEW_CONNECTION_ESTABLISHED, NO_NEW_CONNECTION, TOO_MANY_CLIENTS.
  */
  int checkForNewClients();

  /** Set a single ip address and port number to which all packets will be sent.
      When you use this method, data is forwarded to the reflector running at the
      ip address provided. You will also receive data from that reflector. Thus you
      can create a bidirectional link.

      @return One of QUANTAnet_tcpReflector_c::NEW_CONNECTION_ESTABLISHED, NO_NEW_CONNECTION, TOO_MANY_CLIENTS.
  */
  int setForcedDestination(const char* ipAddr, unsigned short port);

  /** Set the number of seconds to wait for blocking operations to timeout.
      @param seconds
      Number of seconds to wait before timing out a blocking socket call.
  */
  void setBlockingTimeout(const int& seconds);

  /** Send data to all clients connected to the reflector.
   */
  void sendToAll(const char* buffer, int dataSize);

  /** Intercept incoming messages and call a user-defined callback function.
      If you want you can also alter the buffer completely so that the reflector will reflect an
      entirely different message. You can do this by changing the contents of the buffer or
      by replacing the buffer entirely by allocating memory for a new buffer and stuffing it
      with your own data. If you choose to allocate a totally new buffer you must remember
      to deallocate memory for the original
      buffer before substituting it with yours.

      If after your callback function exits you do not wish the reflector to forward
      the contents of the buffer, return with QUANTA_tcpReflector_c::SKIP_DISTRIBUTION. Otherwise
      just return QUANTA_tcpReflector_c::OK.

      Note also that the callback function will also be given a pointer to a
      QUANTAnet_tcpReflectorClient_c object that
      can then be used to send data directly to the client that originally sent the message.
  */
  void intercept(int (*callback) (QUANTAnet_tcpReflectorClient_c *client, char** buffer, int *bufferSize,  void *userData), void* userData);

  /** Intercept any new connections that are formed.
      This allows you to send private data to the newly formed connection before it assumes its data
      reflection duties.
      Callback function will be given a pointer to the QUANTAnet_tcpReflectorClient_c object that
      can then be used to send data directly to the client.
  */
  void interceptNewConnection(void (*callback) (QUANTAnet_tcpReflectorClient_c *newClient, void* userData), void *userData);

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

     (Note: A typical example of sendStats is given in the (QUANTA_DISTRIB_DIR/demos/tcpreflector/ directory)

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
     Port number at which the perfDaemon is running -this is optional. The default port number for perfDaemon is 9500 -so a different port number has to be specified if the perfDaemon is running on a different port.

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int initSendStats(const char* monitorClientIP, int port = PERF_DAEMON_DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */
  void exitSendStats();

protected:
  QUANTAnet_tcpReflectorClient_c **clients;
  QUANTAnet_tcpServer_c *server;
  unsigned short incomingPort;
  char* perfDaemonIP;   //IP of the perfDaemon - used for initsendstats
  int perfDaemonPort;   //PerfDaemon Port
  int perfDaemonStatus; //Status that indicates if the perfDaemon is running
  int (*interceptCallback) (QUANTAnet_tcpReflectorClient_c *newClient, char** buffer, int* bufsize, void *userData);
  void *interceptUserData;

  void (*interceptNewConnectionCallback) (QUANTAnet_tcpReflectorClient_c *newClient, void *userData);
  void *interceptNewConnectionUserData;


  // Distribute the data to all connected clients except the one indicated in
  // clientToIgnore. Specify -1 if want to send to all.
  int distributeDataExceptTo(int clientToIgnore, const char* buf, int incomingSize);

  // Remove a client from the client array.
  void removeClient(int clientNum);

  // Close all client sockets and remove them. Used by destructor.
  // Also closes down and deallocates the server.
  void closeDownSockets();

  // Add a client to the client pool
  int addNewClient(QUANTAnet_tcpReflectorClient_c* newClient);

  int maxNumClients;
};

/** Client specifically for use with QUANTAnet_tcpReflector_c.
    Use this only to communicate with the reflector. Do not use
    the QUANTAnet_tcpClient_c class.

    @see QUANTAnet_tcpReflector_c
*/
class QUANTAnet_tcpReflectorClient_c
{
protected:
  QUANTAnet_tcpClient_c *client;
  int timeOut;
  int connectionAlive;

public:

  //@{
  /// Status ok.
  static const int OK/* = 1*/;

  /// Status failed.
  static const int FAILED/* = 2*/;

  /// Memory allocation error.
  static const int MEM_ALLOC_ERR/* = 3*/;

  /// A non-blocking read had no data available to read.
  static const int NON_BLOCKING_HAS_NO_DATA/* = 7*/;

  /// Connection terminated.
  static const int CONNECTION_TERMINATED/* = 8*/;

  /// Connection not ready for use.
  static const int NOT_READY/* = 9*/;

  /// Connection has data available to read.
  static const int READY_TO_READ/* = 10*/;

  /// Connection is ready to write.
  static const int READY_TO_WRITE/* = 11*/;

  /// Connection has data available to read and is ready to write.
  static const int READY_TO_READ_AND_WRITE/* = 12*/;

  static const int BLOCKING/* = QUANTAnet_tcpClient_c::BLOCKING*/;
  static const int NON_BLOCKING/* = QUANTAnet_tcpClient_c::NON_BLOCKING*/;

  static const int NO_TIME_OUT/* = QUANTAnet_tcpClient_c::NO_TIME_OUT*/;

  //@}

  QUANTAnet_tcpReflectorClient_c();

  /// Use this constructor if you have a pre-opened socket.
  QUANTAnet_tcpReflectorClient_c(QUANTAnet_tcpClient_c *);

  ~QUANTAnet_tcpReflectorClient_c();

  QUANTAnet_tcpClient_c* getClientPtr();

  /** Read data from a reflector (if available).
      @param blockingType set to either QUANTAnet_tcpReflectorClient_c::BLOCKING, NON_BLOCKING.
      @param ret set to a pointer to a char*. Read will allocate memory for the
      buffer and assign it to your pointer. You are responsible for deleteing
      the buffer once you are done with it.
      @param nbytes returns the number of bytes in the buffer.
      @return Either QUANTAnet_tcpReflectorClient_c::MEM_ALLOC_ERR, OK, FAILED, NON_BLOCKING_HAS_NO_DATA, CONNECTION_TERMINATED.
  */
  int read(char** ret, int* nbytes, int blockingType);

  /** Write data to the reflector.
      @param outgoingData set to your buffer containing data.
      @param nbytes set to size of the buffer refered to by outgoingData
      @return Either QUANTAnet_tcpReflectorClient_c::OK,CONNECTION_TERMINATED.
  */
  int write(const char* outgoingData, int* nbytes);

  /** Determines if a socket has data available to read.
      @return Either: QUANTAnet_tcpReflectorClient_c::NOT_READY
      or QUANTAnet_tcpReflectorClient_c::READY_TO_READ
  */
  int isReadyToRead();

  /** Determines if a socket is ready to write.
      @return Either: QUANTAnet_tcpReflectorClient_c::NOT_READY
      or QUANTAnet_tcpReflectorClient_c::READY_TO_WRITE
  */
  int isReadyToWrite();

  /** Determines if a socket is ready to write or read or both.
      @return Either: QUANTAnet_tcpReflectorClient_c::NOT_READY
      or QUANTAnet_tcpReflectorClient_c::READY_TO_WRITE
      or QUANTAnet_tcpReflectorClient_c::READY_TO_READ
      or QUANTAnet_tcpReflectorClient_c::READY_TO_READ_AND_WRITE
  */
  int isReady();

  /// Accessor functions
  //@{
  /** Get the IP address of remote connection.
      If you are a client this returns the ip of the destination server.
      If you are a server this returns the ip of the destination client.
  */
  unsigned int getRemoteIP();

  /** Get the IP address of remote connection.
      If you are a client this returns the ip of the destination server.
      If you are a server this returns the ip of the destination client.
  */
  void getRemoteIP(char* name);

  /// Get port of self.
  int  getSelfPort();

  /// Get port of client.
  int  getRemotePort();
  //@}

  /// Show status of connection in English.
  void showStatus(int status, int nbytes);

  /// Close the current client connection.
  void close();

  /** Connect to a server.
      @param ip IP address to connect to.
      @param port Port to connect to.
      @return Either: QUANTAnet_tcpReflectorClient_c::OK or FAILED
  */
  int connectToServer(const char* ip, int port=QUANTAnet_tcpReflector_c::DEFAULT_PORT);

  /** Set timeout for read and writes. By default no time out is set.
      Setting time out means that if the read or write is unsuccessful
      after a certain amount of time the connection will be terminated.
      This is to prevent you from doing subsequent writes or reads
      from the stream and getting incorrect data.
      In order to re-establish the connection you need to instantiate another
      object of this type.
  */
  void setTimeOut(int timeout=QUANTAnet_tcpReflectorClient_c::NO_TIME_OUT);

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

     (Note: A typical example of sendStats is given in the (QUANTA_DISTRIB_DIR/demos/tcpreflector/ directory)

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
     Port number at which the perfDaemon is running -this is optional. The default port number for perfDaemon is 9500 -so a different port number has to be specified if the perfDaemon is running on a different port.

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int initSendStats(const char* monitorClientIP, int port = PERF_DAEMON_DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */

  void exitSendStats();
};

#endif
