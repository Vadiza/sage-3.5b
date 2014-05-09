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

#ifndef _QUANTADB_C
#define _QUANTADB_C

#include <QUANTA/QUANTAmisc_observer_c.hxx>
#include <QUANTA/QUANTAnet_perfClient_c.hxx> // Needed to define macros.

#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif

// Forward declarations.
class QUANTAnet_tcpReflectorClient_c;
class QUANTAnet_tcpReflector_c;
class QUANTAnet_udpReflector_c;
class QUANTAnet_udp_c;
class QUANTAts_mutex_c;
class keyToolManager_c;

// Default server database directory name.
#define QUANTADB_SERVER_DEFAULT_DB_NAME "QUANTAdbserver"
// Default client database directory name.
#define QUANTADB_CLIENT_DEFAULT_DB_NAME "QUANTAdbclient"

/** QUANTA database server. This server will listen for incoming connections and
    field database requests from QUANTAdb_client_c connections. This database is designed
    mainly to handle small datasets- primarily state information. To retrieve larger datasets consider using the QUANTAnet_remoteFileIO_c calls which are designed to move in excess of
    2Gig data files.
*/
class QUANTAdb_server_c
{
public:
  static const char PUT_MESG/* = 'p'*/;
  static const char FETCH_MESG/* = 'f'*/;
  static const char BLOCKING_FETCH_MESG; // = 'b'
  static const char COMMIT_MESG/* = 'c'*/;

  //@{
  /// Default listening port for the TCP Reflector of the DB Server(for reliable connections)
  static const int DEFAULT_TCP_REFLECTOR_PORT/* = 9500*/;

  ///Default listening port for the UDP Reflector of the DB Server
  static const int DEFAULT_UDP_REFLECTOR_PORT /* = 9550*/;

  /// Default maximum number of clients that the server can manage.
  static const int DEFAULT_MAX_CLIENTS/* = 64*/;

  /// Status ok.
  static const int OK/* = 1*/;

  /// Status failed.
  static const int FAILED/* = 0*/;

  /// Memory allocation error.
  static const int MEM_ALLOC_ERR/* = 3*/;

  /// Server cannot manage any more clients.
  static const int TOO_MANY_CLIENTS/* = 4*/;

  /// Key does not exist
  static const int KEY_DOES_NOT_EXIST /* = -1 */;
  //@}

  ~QUANTAdb_server_c();

  /** Initialize server.
      @param defaultDBName database directory name. Default name is QUANTAdbserver.
      @param incomingPort db server's (tcp reflector's) listening port. Default port 9500.
      @param maxClients maximum number of clients that may connect to the server. Default is 64 clients.
      @param udpReflectorPort udp reflector listening port. Default port is 9550

      @return MEM_ALLOC_ERR, FAILED or OK.
  */
  int init(char *defaultDBName = QUANTADB_SERVER_DEFAULT_DB_NAME, int incomingPort = QUANTAdb_server_c::DEFAULT_TCP_REFLECTOR_PORT, int maxClients = QUANTAdb_server_c::DEFAULT_MAX_CLIENTS, int udpReflectorPort = QUANTAdb_server_c::DEFAULT_UDP_REFLECTOR_PORT);

  /** Handle connections and process incoming requests. Do this in a while loop as frequently as possible to ensure that requests are processed rapidly.
      @return OK, FAILED, TOO_MANY_CLIENTS
  */
  int process();

  // Handle any new connections. This does nothing.
  static  void newConnectionHandler(QUANTAnet_tcpReflectorClient_c *newClient, void* userData);

  // Handle incoming commands.
  static int newDataHandler(QUANTAnet_tcpReflectorClient_c * client, char** buffer, int *bufferSize, void *userData);

  // Handler for incoming commands received by the udp reflector
  static int udpDataHandler(QUANTAnet_udp_c* udpClient, char** buffer, int* bufferSize,void* userData);

  //Functions for performance monitoring

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
     Port number at which the perfDaemon is running -this is optional. The default port number for perfDaemon is 9500 -so a different port number has to be specified if the perfDaemon is running on a different port.

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int initSendStats(char* monitorClientIP, int port = PERF_DAEMON_DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */
  void exitSendStats();

protected:
  QUANTAnet_tcpReflector_c * server;
  QUANTAnet_udpReflector_c*  udpreflector;
  keyToolManager_c *keyToolManager;

  //TCP Reflector listening port
  int incomingPort;

  // Handle PUT commands.
  void handlePut(char *buf, int size);

  // Handle FETCH commands.
  void handleFetch(QUANTAnet_tcpReflectorClient_c *client, char *buf, int size);

  //Handle BlOCKING_FETCH commands
  void handleBlockingFetch(QUANTAnet_tcpReflectorClient_c *client, char *buf, int size);

  // Handle COMMIT command.
  void handleCommit(QUANTAnet_tcpReflectorClient_c *client, char *buf, int size);

  // Send a put message
  void sendPutMessage(QUANTAnet_tcpReflectorClient_c *client, char* pathname, int pathsize, char* keyname, int keysize);


};

/** QUANTA database client. This client will connect to database servers written with QUANTAdb_server_c. Currently only a few basic commands have been implemented. More will be added with time.
 */
class QUANTAdb_client_c  : public QUANTAmisc_subject_c
{
public:

  //@{
  /// Status failed.
  static const int FAILED/* = 0*/;
  /// Status ok.
  static const int OK/* = 1*/;

  /// Memory allocation error.
  static const int MEM_ALLOC_ERR/* = 2*/;

  ///  Connection terminated.
  static const int CONNECTION_TERMINATED/* = 3*/;

  /// Action ignored. See reconnect()
  static const int IGNORED/* = 4*/;

  /// Use unreliable UDP for data transmission (make sure the datasize is less than 1K)
  static  const int UNRELIABLE_CONNECTION /* = 5*/;

  /// Use reliable TCP for data transmission
  static  const int RELIABLE_CONNECTION /* = 6*/;

  /// Maximum udp packet size allowed (1 KB)
  static const int MAX_UDP_PACKET_SIZE /* = 1K*/;

  /// The key does not exist - used by blocking fetch
  static const int KEY_DOES_NOT_EXIST /* = -1 */;

  //@}

  QUANTAdb_client_c();
  ~QUANTAdb_client_c();

  /** Initialize client by connecting to remote server.
      If you specify serverIP as NULL then you are making no network connections. Ie you are using the client database in local single user mode.

      @param serverIP IP address of QUANTAdb_server_c.
      @param serverPort Listening port of db_server's tcp reflector.Default is 9500.
      @param defaultDBName Database directory name. Default is QUANTAdbclient.
      @param udp_reflector_port Listening port of db_server's udp reflector. Default is 9550.

      @return OK, FAILED, MEM_ALLOC_ERR
  */
  int init(char *serverIP, int serverPort= QUANTAdb_server_c::DEFAULT_TCP_REFLECTOR_PORT,  char* defaultDBName = QUANTADB_CLIENT_DEFAULT_DB_NAME, int udp_reflector_port = QUANTAdb_server_c::DEFAULT_UDP_REFLECTOR_PORT);

  /** Fetch data from server. This is a non-blocking call so once the fetch command has been issued it will return. It is then up to the server to respond eventually.
      @param pathname Path name
      @param keyname Key name
      @return  FAILED, OK
  */
  int fetch(char *pathname, char *keyname);

  /** Fetch data from server and block for the reply. This is the same as the normal fetch call except that its blocking and receives a reply even if the key does not exist(saying the key does not exist!)
      @param pathname Path name
      @param keyname Key name
      @return  OK, FAILED, KEY_DOES_NOT_EXIST
  */
  int blockingFetch(char *pathname, char *keyname);
  /** Set a trigger so that when data arrives it will fire a callback. Callbacks
      are fired after notification of observers.
      @param callback callback function pointer. Set this to NULL for no trigger.
      @param userData user data to pass onto callback when it is fired.
  */
  void trigger(void (*callback) (char* pathname, char* keyname, void *userData), void* userData);

  /** Put data into a key. The users can also make use of a UDP connection for the data transmission by using QUANTAdb_client_c::UNRELIABLE_CONNECTION (the default is a reliable tcp connection). Please note that the datasize for UDP should be less than 1K. If the datasize is more than 1K, the data is sent via a TCP connection

      @param pathname Path name
      @param keyname Key name
      @param data data to put
      @param datasize size of data buffer
      @param typeOfConnection the type of connetion to be used
      @return MEM_ALLOC_ERR, OK.
  */
  int put(char* pathname, char* keyname, char* data, int datasize, int typeOfConnection = QUANTAdb_client_c::RELIABLE_CONNECTION);

  /** Get data in a key.
      @param pathname Path name
      @param keyname Key name
      @param datasize returns size of data
      @return memory buffer containing data. Remember to delete the buffer
      when you are done using it.
  */
  char* get(char* pathname, char* keyname, int* datasize);

  /** Process the client. Do this as frequently as possible and in a while loop.
      @return OK, CONNECTION_TERMINATED
  */
  int process();

  /** Reconnect to server on broken connection.
      If the result of process() call is CONNECTION_TERMINATED you can
      try to reconnect to the server.
      @param servername set this to NULL to have it reconnect using the
      last server name.
      @param port set this to 0 to have it reconnect using the last server port.
      @return OK, FAILED, MEM_ALLOC_ERR, IGNORED. IGNORED is returned if
      you are trying to do a reconnect but originally you were running
      in non-networked mode (ie you called init() with a NULL serverIP name).
  */
  int reconnect(char* servername = NULL, int port = 0);

  /** Commit data in local database.
      @param pathname pathname
      @param keyname keyname
      @return OK or MEM_ALLOC_ERR
  */
  int commit(char* pathname, char* keyname);

  /** Commit data in server database.
      @param pathname pathname
      @param keyname keyname
      @return OK or FAILED.
  */
  int remoteCommit(char* pathname, char* keyname);

  /** Get the pathname of the key that has triggered a notify event.
      This is mainly used by observers that have attached to this object.
  */
  char* getNotifyPathname() {return notifyPathname;}

  /** Get the keyname of the key that has triggered a notify event.
      This is mainly used by observers that have attached to this object.
  */
  char* getNotifyKeyname() {return notifyKeyname;}


  //Functions for performance monitoring

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
     Port number at which the perfDaemon is running -this is optional. The default port number for perfDaemon is 9500 -so a different port number has to be specified if the perfDaemon is running on a different port.

     @return
     Either QUANTAnet_perfMonitor_c::OK or QUANTAnet_perfMonitor_c::FAILED
  */

  int initSendStats(char* monitorClientIP, int port = PERF_DAEMON_DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */
  void exitSendStats();

protected:
  char *notifyPathname;
  char *notifyKeyname;

  keyToolManager_c *keyToolManager;
  QUANTAnet_tcpReflectorClient_c *client;
  QUANTAnet_udp_c* udpClient;
  /** Clientmutex is used to try and prevent
      multiple threads from using the socket at the same time or
      using the keytool database.
      Callbackmutex is used to make sure multiple callbacks
      do not fire at the same time since there is no guarantee
      that the user will write a thread-safe callback function.
  */
  QUANTAts_mutex_c *clientmutex, *callbackmutex;

  void *callbackUserData;
  void (*triggerCallback) (char* pathname, char* keyname, void* userData);
  char itsServerName[256];
  int itsServerPort;
  int udpReflectorPort;

  // Handle incoming messages
  void handleMessage(char* data, int dataSize);

  // Handle incoming PUT messags
  void handleIncomingPut(char* data, int datasize);

  //Handle a reply to a blocking fetch
  int handleFetchReply(char* data, int dataSize);
};


#endif
