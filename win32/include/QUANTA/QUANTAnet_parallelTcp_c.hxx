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

#ifndef _QUANTAPLUS_PARALLEL_TCP_C
#define _QUANTAPLUS_PARALLEL_TCP_C

#include <QUANTA/QUANTAnet_socketbase_c.hxx>
#include <QUANTA/QUANTAnet_datapack_c.hxx>

/**
   Parallel socket class. This enables us to transfer a stream of data over
   multiple sockets simultaneously.

   Why is this faster than a single socket connection? The TCP protocol requires
   acknowledgement from the other end everytime it sends a packet, thus it waits
   for someting on every send.  With parallel sockets, while one socket is waiting,
   another socket connection can send data.  We therefore eliminate idle time.

   If you are writing a server you need to instantiate a QUANTAnet_parallelTcpServer_c object.
   Likewise if you are writing a client you need to instantiate a QUANTAnet_parallelTcpClient_c
   object.

   Server setup:  First, instantiate the QUANTAnet_parallelTcpServer_c class. Then call
   this object's init() method with the desired port number as parameter.
   After this, we call QUANTAnet_parallelTcpServer_c::checkForNewConnections(PortNumber).
   This is a blocking call that waits for the client to make a
   QUANTAnet_parallelTcpClient_c::connectToServer()
   call. checkForNewConnections() returns a pointer to a QUANTAnet_parallelTcpClient_c object,
   which you can use to communicate with the client. Note that you do not explicitly
   instantiate a QUANTAnet_parallelTcpClient_c object when you are using the server class.
   You can just declare a pointer to the client object and assign to this variable the return
   value of QUANTAnet_parallelTcpServer_c::checkForNewConnections function.

   Client setup:  We instantiate the QUANTAnet_parallelTcpClient_c class and call the
   QUANTAnet_parallelTcpClient_c::connectToServer(MachineName,PortNumber,NumberOfSockets) method.
   We then use this object to do a write and read.

   @author: Stuart Bailey, cavern@evl

   @version: 2/22/2000
*/

class QUANTAnet_parallelTcpClient_c : public QUANTAnet_socketbase_c
{
public:
  QUANTAnet_parallelTcpClient_c();
  virtual ~QUANTAnet_parallelTcpClient_c();

  ///Parallel socket class return values
  //@{
  /// Operation successful
  static const int OK    /* = 1*/;
  /// Operation failed
  static const int FAILED/* = 0*/;
  //@}
  /**
     After returning from checkForNewConnections() or after calling connectToServer(),
     you can now call write() to send Length bytes of data starting at Buffer

     @return
     Either QUANTAnet_parallelTcpClient_c::OK or QUANTAnet_parallelTcpClient_c::FAILED

     @param Buffer
     A pointer to the buffer to be sent.

     @param Length
     Number of bytes to be sent.
  */
  int write( char* buffer, int* length );

  /**
     After returning from checkForNewConnections() or after calling connectToServer(),
     you can now call write() to send Length bytes of data starting at Buffer

     @return
     Either QUANTAnet_parallelTcpClient_c::OK or QUANTAnet_parallelTcpClient_c::FAILED

     @param Buffer
     A pointer to the buffer that will store the received data.

     @param Length
     Number of bytes to be received.
  */
  int read( char* buffer, int* length );


  /**
     Close down the multiple socket handles
  */
  void close();

  /**
     We instantiate a QUANTAnet_parallelTcpClient_c object and calls this
     connectToServer() method to connect to machine named "RemoteName" on port Port.
     We specify the number of simultaneous socket connections in
     the Size parameter. After this call is successfully called, you can then
     use read and write methods.

     @param RemoteName
     A character string, name of the machine where the remote server is.

     @param Port
     Port number which the remote server is listening to.

     @param Size
     The number of simultaneous sockets we are to use.

     @return
     Either QUANTAnet_parallelTcpClient_c::OK or QUANTAnet_parallelTcpClient_c::FAILED

  */
  int connectToServer(char *RemoteName, int RemotePort, int Size);

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

public:
  /* These functions are provided to be used in QUANTAnet_parallelTcpServer_c only. */
  void setSockets(int* Sockets, int NumSockets);/* {
                                                   sockets = Sockets;
                                                   numSockets = NumSockets;
                                                   for (int i = 0; i < NumSockets; i++) {
                                                   if (sockets[i] > maxSockDesc) maxSockDesc = sockets[i];
                                                   }
                                                   pcReadPtr = new char *[NumSockets];
                                                   pcWritePtr = new char *[NumSockets];
                                                   piBytesLeft = new int[NumSockets];
                                                   }*/

protected:
  /* Not to be called by user */
  void makeNonBlocking(int Sockfd);
  int connectToClient(char* RemoteName, unsigned short RemotePort,
                      int& SocketInfo);

  int* sockets;
  int maxSockDesc;
  int numSockets;
  char** pcReadPtr;
  char** pcWritePtr;
  int* piBytesLeft;
  fd_set fdSet;
  struct sockaddr_in remoteAddr;
  struct sockaddr_in localAddr;
};

/**
   If you are writing a server you need to instantiate a QUANTAnet_parallelTcpServer_c object.
   Likewise if you are writing a client you need to instantiate a QUANTAnet_parallelTcpClient_c
   object.

   Server setup:  First, instantiate the QUANTAnet_parallelTcpServer_c class. Then call
   this object's init() method with the desired port number as parameter.
   After this, we call QUANTAnet_parallelTcpServer_c::checkForNewConnections(PortNumber).
   This is a blocking call that waits for the client to make a
   QUANTAnet_parallelTcpClient_c::connectToServer()
   call. checkForNewConnections() returns a pointer to a QUANTAnet_parallelTcpClient_c object,
   which you can use to communicate with the client. Note that you do not explicitly
   instantiate a QUANTAnet_parallelTcpClient_c object when you are using the server class.
   You can just declare a pointer to the client object and assign to this variable the return
   value of QUANTAnet_parallelTcpServer_c::checkForNewConnections function.

   Client setup:  We instantiate the QUANTAnet_parallelTcpClient_c class and call the
   QUANTAnet_parallelTcpClient_c::connectToServer(MachineName,PortNumber,NumberOfSockets) method.
   We then use this object to do a write and read.

*/
class QUANTAnet_parallelTcpServer_c : public QUANTAnet_socketbase_c
{
public:
  //@{
  /// Status ok.
  static const int OK/* = 1*/;

  /// Status failed.
  static const int FAILED/* = 0*/;
  //@}


  QUANTAnet_parallelTcpServer_c() {};
  virtual ~QUANTAnet_parallelTcpServer_c();

  /** Open the server on a port.
      Typically after this call you sit in a loop and call
      checkForNewConnections to wait for incoming connections.
      @return FAILED if failed, OK if success
  */
  int init(int port);

  /// Close the server port.
  void close();

  /** Check to see if there is a request from clients for connection.
   * If yes then return a QUANTAnet_parallelTcpClient_c object which you can then use
   * to talk to the client.
   * @see QUANTAnet_parallelTcpClient_c class.
   */
  QUANTAnet_parallelTcpClient_c *checkForNewConnections();

  /// Get port of self.
  int getSelfPort() { return ntohs(serverInfo.sin_port); };

protected:
  void makeNonBlocking(int Sockfd);
  int connectToClient(unsigned long RemoteAddr, unsigned short RemotePort,
                      int& SocketInfo);

  struct sockaddr_in  serverInfo;
  int sockfd;
};

#endif
