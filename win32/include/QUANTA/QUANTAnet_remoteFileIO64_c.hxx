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

#ifndef _QUANTAnet_remoteFileIO64_c
#define _QUANTAnet_remoteFileIO64_c

#include <QUANTA/QUANTAnet_extendedTcp_c.hxx>

#ifdef WIN32
#include <stdlib.h>
#define MAXPATHLEN  MAX_PATH
#else
#include <sys/param.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/** This class acts as a server for performing remote upload and download of files.
    First call the init() call and then inside a while loop call checkForNewRequests().
    Use QUANTAnet_remoteFileIOClient_c to connect to this server and perform the upload
    and download operations.

    Note: currently there exists a strange behavior in threads that causes the
    network I/O to improperly complete. Hence checkForNewRequests() currently
    does not create a new thread for each client that makes an IO request. All
    clients must wait in line.
*/
class QUANTAnet_remoteFileIOServer64_c {
public:
  /** Default port number to listen on.
      Default is 8000.
  */
  static const int DEFAULT_PORT/* = 8000*/;

  /// Create a thread to handle each client request.
  static const int THREADED/* = 1*/;

  /// Do not create a thread to handle each  client request.
  static const int NON_THREADED/* = 0*/;

  /** Initialize server.
      @param inPort
      The local port number to listen on (default is 8000)

      @param neighborServer
      This should be a valid internet address of a host you want to connect to if
      files are not present on the local server

      @param neighborPort
      This is the remote port number (default is 8000)

      @param storeLocally
      storeLocally 0 means that the file downloaded from the neighborServer will not be saved locally;
      1 means that it is saved.
      By default this parameter is 1.

      @return
      FAILED(FAILED = 0) if init() failed,
      OK(OK = 1) if successful
  */
  int init(int inPort=QUANTAnet_remoteFileIOServer64_c::DEFAULT_PORT,
           char* neighborServer = NULL,
           int neighborPort = QUANTAnet_remoteFileIOServer64_c::DEFAULT_PORT,
           int storeLocally = 1);

  /** Check for incoming requests.
      Recommend doing this in a while loop.
      @param threaded set this to THREADED or NON_THREADED.
      Default is NON_THREADED.
      Setting it to non-threaded will mean each request is
      processed sequentially so any pending requests must
      wait in the queue. Setting it to threaded means each
      incoming request spawns a thread to handle the request.
  */
  void process(int threaded = QUANTAnet_remoteFileIOServer64_c::NON_THREADED);

  void setRemotePort(int port=QUANTAnet_remoteFileIOServer64_c::DEFAULT_PORT) {
    neighborPortNumber = port;
  }
  void setNeighborServer(char* ServerName) {
    if (ServerName) strcpy(neighborServerName, ServerName);
    else neighborServerName[0] = '\0';
  }
  void setStore(int StoreLocally) { store = StoreLocally; }

  long getFileSize(FILE* readFile);

  static const int TRANSFERSIZE/* = 30000*/;

private:
  QUANTAnet_extendedTcpServer_c server;
  char neighborServerName[MAXPATHLEN];
  int neighborPortNumber;
  int store;

  static int getHostIPNumber(char* hostNameOrIP);
  static void* threadHandler(void*);
};

/** Upload and download files to and from a remote server. This client is designed specifically to connect to QUANTAnet_remoteFileIOServer64_c.
 */
class QUANTAnet_remoteFileIOClient64_c {
public:

  //@{
  /// Status ok
  static const int OK/* = 1*/;

  /// Status failed
  static const int FAILED/* = 0*/;

  /// Cannot connect to remote server.
  static const int CANNOT_CONNECT/* = 2*/;

  /// Cannot open local file for reading.
  static const int CANNOT_READ_LOCAL_FILE/* = 3*/;

  /// CAnnot write to local file.
  static const int CANNOT_WRITE_LOCAL_FILE/* = 3*/;

  /// Received invalid command from server.
  static const int INVALID_COMMAND/* = 4*/;

  /// Memory allocation error.
  static const int MEM_ALLOC_ERR/* = 5*/;

  /// Remote file error.
  static const int FILE_ERROR/* = 6*/;

  /// Earlier date/time
  static const int EARLIER/* = 7*/;

  /// Later date/time
  static const int LATER/* = 8*/;

  /// Same date/time
  static const int SAME/* = 9*/;

  //@}

  QUANTAnet_remoteFileIOClient64_c();

  /// Set port number to connect to.
  void setRemotePort(int port = QUANTAnet_remoteFileIOServer64_c::DEFAULT_PORT);

  /** Upload a file to the remote server. If upload fails because it is unable to
      write to the remote server or a network error occurs this function will return with a FAILED.
      @param ipAddress is IP address of remote server.
      @param localFileName is local file to send.
      @param remoteFileName is filename to use at the remote end.
      @return OK, FAILED< CANNOT_READ_LOCAL_FILE, INVALID_COMMAND, CANNOT_CONNECT
  */
  int upload(char* ipAddress,char* localFileName, char* remoteFileName);

  /** Download a file from the remote server. If the remote file is not found or
      a network error occurs, this function will return with a failed.
      @param ipAddress is IP address of remote server.
      @param localFileName is local file write to.
      @param remoteFileName is remote file to get.
      @return OK, FAILED, CANNOT_WRITE_LOCAL_FILE, INVALID_COMMAND, CANNOT_CONNECT
  */
  int download(char* ipAddress, char* localFilename, char* remoteFileName);

  /** Retrieve a remote file's date and time stamp. Date/Time is in GMT.
      @return OK, FAILED - if command failed; FILE_ERROR - if remote file cannot be accessed.
      Values of day, month, year, etc will return all -1s if FILE_ERROR occurs.

  */
  int getRemoteDateTime(char* ipAddress, char* remoteFileName,
                        int *remDay, int *remMon, int *remYear,
                        int* remHour, int* remMin, int* remSec);

  /** Retrieve a local file's date and time stamp. Date/Time is in GMT.
      @return OK or FILE_ERROR - if local file could not be accessed.
      Values of day, month, year, etc will return all -1s if FILE_ERROR occurs.
  */
  static int getLocalDateTime(char* filename,
                              int *day,
                              int *month,
                              int *year,
                              int *hour,
                              int *minute,
                              int *second);

  /** Compare time/date1 with time/date2.
      @return EARLIER - if time/date1 is earlier than time/date2
      LATER - if time/date1 is later than time/date2
      SAME - if time/date1 and 2 are same.
  */

  int compareDateTime(int day1, int month1, int year1,
                      int hour1, int minute1, int second1,
                      int day2, int month2, int year2,
                      int hour2, int minute2, int second2);

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
  int remotePort;
  QUANTAnet_extendedTcpClient_c client;
  QUANTAnet_perfMonitor_c pmonitor;   //for performance monitoring
};


/*

  FORMAT:There are no spaces between fields

  UPLOAD
  ------
  CLIENT -> SERVER
  u fileNameLen fileName dataSize data

  DOWNLOAD
  --------
  CLIENT -> SERVER
  d fileNameLen  fileName
  SERVER->CLIENT
  d (dataSize, timeStamp) data

  CHECK REMOTE TIMESTAMP ON FILE
  -----------------------------
  CLIENT->SERVER
  c fileNameLen fileName
  SERVER->CLIENT
  c (time, timestamp)

  REPLY:SERVER->CLIENT (For all commands)

  File Not found:  'n'
  Invalid command: 't'

*/
#endif
