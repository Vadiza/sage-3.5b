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

#ifndef _QUANTAPLUS_PERF_DAEMON_C
#define _QUANTAPLUS_PERF_DAEMON_C

#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif

#include <QUANTA/QUANTAnet_perfClient_c.hxx>

class QUANTAnet_tcpServer_c;

struct PerfDaemonClient
{
  QUANTAnet_perfDaemonClient_c *Client;
  int IsPerfClient;
  int Started;
};

/** QUANTA performance monitoring daemon class. This class treats TCP
    streams as discrete packets/messages that can be reflected to multiple
    connected performance monitoring clients. Clients must be created using
    QUANTAnet_perfDaemonClient_c. Hence do NOT attempt to use
    QUANTAnet_tcpClient_c to connect to a perf daemon class. This class has
    2 main functions: checkForNewClients() and process(). checkForNewClients,
    as the name suggests, checks to see if any new clients wish to connect
    to the reflector. Process() does the actual work of data reflection.

    checkForNewClients is called everytime process() is called. If you want
    the check to be done more frequently you need to do it yourself. Similarly
    if you want the process() call done more frequently it is up to you to do it.
    I recommend threading those two off and setting up a mutex so that you do
    not do process() and checkForNewClients() at the same time.

    @author cavern@evl.uic.edu
    @version 3/28/2000

*/
class QUANTAnet_perfDaemon_c
{
public:
  /**
     Constructor of performance monitoring daemon class.

     @param File
     Optional parameter. If file pointer is given here, it will be used
     to save all log information.
  */
  QUANTAnet_perfDaemon_c(FILE* File = NULL);
  ~QUANTAnet_perfDaemon_c();

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

  static const int DEFAULT_PORT/* = 9500*/;
  //@}

  /** Initialize the reflector.
      @param incomingPort is listening port for incoming connections. Default is 7000.
      @param maxClients is the max number of clients the reflector will manage.
      @return Either QUANTAnet_perfDaemon_c::OK,FAILED,MEM_ALLOC_ERR.
  */
  int init(int incomingPort=PERF_DAEMON_DEFAULT_PORT, int maxClients = 64);

  /** Call this within a while loop to let the reflector continuously
      do its processing.
      @return Either QUANTAnet_perfDaemon_c::OK,MEM_ALLOC_ERR
  */
  int process();

  /** Call this as often as you wish to check for new clients.
      Note. If you do this in a separate thread then you must set up a mutex
      so that you do not call the proces() call and this call at the same time.
      The process() call itself has imbedded in it 1 check for each time you
      call it.
      @return Either QUANTAnet_perfDaemon_c::NEW_CONNECTION_ESTABLISHED, NO_NEW_CONNECTION, TOO_MANY_CLIENTS.
  */
  int checkForNewClients();

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
      QUANTAnet_perfDaemonClient_c object that
      can then be used to send data directly to the client that originally sent the message.
  */
  void intercept(int (*callback) (QUANTAnet_perfDaemonClient_c *client, char** buffer,
                                  int *bufferSize,  void *userData), void* userData);

  /** Intercept any new connections that are formed.
      This allows you to send private data to the newly formed connection before it assumes its data
      reflection duties.
      Callback function will be given a pointer to the QUANTAnet_perfDaemonClient_c object that
      can then be used to send data directly to the client.
  */
  void interceptNewConnection(void (*callback) (QUANTAnet_perfDaemonClient_c *newClient, void* userData), void *userData);

  int sendToAll(char* buf, int incomingSize);

private:
  PerfDaemonClient **clients;
  QUANTAnet_tcpServer_c *server;
  unsigned short incomingPort;
  FILE* LogFile;

  int (*interceptCallback) (QUANTAnet_perfDaemonClient_c *newClient, char** buffer,
                            int* bufsize, void *userData);
  void *interceptUserData;

  void (*interceptNewConnectionCallback) (QUANTAnet_perfDaemonClient_c *newClient, void *userData);
  void *interceptNewConnectionUserData;

  // Distribute the data to all connected performance monitoring clients.
  // Specify -1 if want to send to all.
  int distributeDataToPerfClients(char* buf, int incomingSize);

  // Remove a client from the client array.
  void removeClient(int clientNum);

  // Close all client sockets and remove them. Used by destructor.
  // Also closes down and deallocates the server.
  void closeDownSockets();
  int maxNumClients;
};

#endif
