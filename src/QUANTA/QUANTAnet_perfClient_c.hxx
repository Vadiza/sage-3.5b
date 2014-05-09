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

#ifndef _QUANTAPLUS_PERF_CLIENT
#define _QUANTAPLUS_PERF_CLIENT

#define PERF_DAEMON_DEFAULT_PORT  9500
#define REGISTER_PERF_CLIENT "$$QUANTA_PERF_DAEMON_CLASS$$"
#define SEND_STAT_CLIENT     "$$SEND_STAT_PERF_CLIENT$$$$$"
#define REGISTER_PERF_CLIENT_LENGTH 28 // length of REGISTER_PERF_CLIENT string
#define SEND_STAT_CLIENT_LENGTH 28 // length of SEND_STAT_CLIENT string
#define CLOSING_STAT_CLIENT  "$$CLOSING_STAT_PERF_CLIENT$$"
#define CLOSING_STAT_CLIENT_LENGTH 28
#define CLOSING_PERF_CLIENT  "$$CLOSING_PERF_DAEMON_CLASS$"
#define CLOSING_PERF_CLIENT_LENGTH 28

class QUANTAnet_tcpClient_c;

/** This class is a very low-level class that is designed for specific uses.
    If what you want to do is checking performance characteristics of your
    application, you don't have to use this class at all. All QUANTA
    classes are updated with instrumentation features.

    QUANTAnet_perfDaemonClient_c is a specially designed class
    specifically for use with QUANTAnet_perfDaemon_c class or performance
    monitoring daemon application that instantiates and utilizes QUANTAnet_perfDaemon_c class.
    This class can be used if you want to do one of the following:

    First, it can be used to communicate directly with performance daemon. If you want
    to send special messages to the daemon, you can use this class for the purpose.
    Second, you are developing real time performance monitoring applications like QoSiMoto.

    If you want to send your own special messages to performance monitoring daemon,
    you have to simply instantiate this class and use provided methods to send special
    messages to the performance daemon. If your interest is in monitoring the performance
    characteristics of other applications then you need to register as performance
    monitoring client when you instantiate this class. Read the description of constructors
    to find out how to register your application as performance monitoring client.
*/
class QUANTAnet_perfDaemonClient_c
{
private:
  QUANTAnet_tcpClient_c *client;
  int timeOut;
  int connectionAlive;
  int perfClient;

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

  /** Constructor
      @param isPerfClient 1 if your application is designed to monitor
      the performance characteristics of other applications like QoSiMoto.
      Otherwise, specify 0.
  */
  QUANTAnet_perfDaemonClient_c(int isPerfClient = 0);

  /// Use this constructor if you have a pre-opened socket
  QUANTAnet_perfDaemonClient_c(QUANTAnet_tcpClient_c*, int isPerfClient = 0);
  ~QUANTAnet_perfDaemonClient_c();

  /** Read data from a reflector (if available).
      @param blockingType set to either QUANTAnet_perfDaemonClient_c::BLOCKING, NON_BLOCKING.
      @param ret set to a pointer to a char*. Read will allocate memory for the
      buffer and assign it to your pointer. You are responsible for deleteing
      the buffer once you are done with it.
      @param nbytes returns the number of bytes in the buffer.
      @return Either QUANTAnet_perfDaemonClient_c::MEM_ALLOC_ERR, OK, FAILED, NON_BLOCKING_HAS_NO_DATA, CONNECTION_TERMINATED.
  */
  int read(char** ret, int* nbytes, int blockingType);

  /** Write data to the reflector.
      @param outgoingData set to your buffer containing data.
      @param nbytes set to size of the buffer refered to by outgoingData
      @return Either QUANTAnet_perfDaemonClient_c::OK,CONNECTION_TERMINATED.
  */
  int write(const char* outgoingData, int* nbytes);

  /** Determines if a socket has data available to read.
      @return Either: QUANTAnet_perfDaemonClient_c::NOT_READY
      or QUANTAnet_perfDaemonClient_c::READY_TO_READ
  */
  int isReadyToRead();

  /** Determines if a socket is ready to write.
      @return Either: QUANTAnet_perfDaemonClient_c::NOT_READY
      or QUANTAnet_perfDaemonClient_c::READY_TO_WRITE
  */
  int isReadyToWrite();

  /** Determines if a socket is ready to write or read or both.
      @return Either: QUANTAnet_perfDaemonClient_c::NOT_READY
      or QUANTAnet_perfDaemonClient_c::READY_TO_WRITE
      or QUANTAnet_perfDaemonClient_c::READY_TO_READ
      or QUANTAnet_perfDaemonClient_c::READY_TO_READ_AND_WRITE
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

  /// Show statistics of a connection.
  void showStats();

  /// Close the current client connection.
  void close();

  /** Connect to a server.
      @param ip IP address to connect to.
      @param port Port to connect to.
      @return Either: QUANTAnet_perfDaemonClient_c::OK or FAILED
  */
  int connectToServer(const char* ip, int port=PERF_DAEMON_DEFAULT_PORT);

  /** Set timeout for read and writes. By default no time out is set.
      Setting time out means that if the read or write is unsuccessful
      after a certain amount of time the connection will be terminated.
      This is to prevent you from doing subsequent writes or reads
      from the stream and getting incorrect data.
      In order to re-establish the connection you need to instantiate another
      object of this type.
  */
  void setTimeOut(int timeout=QUANTAnet_perfDaemonClient_c::NO_TIME_OUT);
};
#endif
