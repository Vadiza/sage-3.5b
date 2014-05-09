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

#ifndef _QUANTAPLUS_RPC_C
#define _QUANTAPLUS_RPC_C

#include <QUANTA/QUANTAnet_datapack_c.hxx>
#include <QUANTA/QUANTAnet_tcp_c.hxx>

class QUANTAnet_rpcHandle_c
{
public:
  char *rpcName;
  int (*callback) (char* indata, int inlen, char** outdata, int *outlen);
  QUANTAnet_rpcHandle_c();
};


/**
   Remote Procedure Call Manager class. This class handles incoming RPC calls.
*/

class QUANTAnet_rpcManager_c
{
public:
  //@{
  /** Default setting for the number of RPCs a manager can handle.
      Default is 64
  */
  static const int DEFAULT_RPCS/* = 64*/;

  /** Default RPC listening port. Default is 9000.
   */
  static const int DEFAULT_PORT/* = 9000*/;

  /// RPC status ok
  static const int OK/* = 1*/;

  /// RPC run out of space for handlers.
  static const int CANNOT_MANAGE_ANY_MORE/* = 4*/;

  /// RPC not found in manager
  static const int RPC_NOT_FOUND/* = 3*/;

  /// Mem alloc error occured in RPC call
  static const int MEM_ALLOC_ERR/* = 2*/;

  /// RPC call failed
  static const int FAILED/* = 0*/;
  //@}

  QUANTAnet_rpcManager_c();

  /** Initialize RPC manager.
      @param maxRPCs maximum number of RPCs the manager can handle.
      @param inPort listening port of manager.
      @return FAILED or OK
  */
  int init(int maxRPCs = QUANTAnet_rpcManager_c :: DEFAULT_RPCS,
           int inPort = QUANTAnet_rpcManager_c :: DEFAULT_PORT);

  /** Manage a procedure.
      @param procedureName name of procedure.
      @param callback user callback to call in response to RPC
      @return MEM_ALLOC_ERR, OK, CANNOT_MANAGE_ANY_MORE
  */
  int manage(char* procedureName, int (*callback) (char *indata, int inlen,
                                                   char **outdata, int *outlen));

  /** Unmanage a procedure.
      @return OK or RPC_NOT_FOUND
  */
  int unmanage(char* procedureName);

  /** Process RPCs. Do this in a while loop.
   */
  void process();


  //for performance monitoring

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
  int maxNumRPCs;
  QUANTAnet_rpcHandle_c *handleList;
  QUANTAnet_tcpServer_c server;

  // Reply to an RPC. Returns OK or FAILED
  int replyRPC(QUANTAnet_tcpClient_c *client, char* returnValue, int returnSize);

  // Find an RPC in the manager list
  QUANTAnet_rpcHandle_c *findRPC(char*);

  // Handle incoming RPC request. Returns OK or FAILED
  int handleRPC(QUANTAnet_tcpClient_c *client);

  //For performance monitoring
  QUANTAnet_perfMonitor_c pmonitor;
};

/** Remote Procedure Caller class. Use this to call the remote RPC manager.
 */
class QUANTAnet_rpcCaller_c
{
public:
  //@{
  /// Use to set RPCs to never time out.
  static const int NEVER_TIME_OUT/* = QUANTAnet_tcpClient_c::NO_TIME_OUT*/;
  /// RPC Failed
  static const int FAILED/* = 0*/;

  /// RPC Ok
  static const int OK/* = 1*/;

  /// Mem alloc error in RPC
  static const int MEM_ALLOC_ERR/* = 2*/;
  //@}

  /** Initialize RPC.
      @param remoteProcedureName name of remote procedure
      @param ipAddress ip address of rpc server.
      @param destPort destination port of rpc server.
      @return OK or MEM_ALLOC_ERR
  */
  int init(char *remoteProcedureName, char* ipAddress, int destPort=QUANTAnet_rpcManager_c::DEFAULT_PORT);

  /** Remote procedure call.
      @param outdata outgoing data
      @param outlen outgoing data size
      @param retdata incoming return data
      @param retlen incoming return data size
      @return OK or MEM_ALLOC_ERR or FAILED
  */
  int call(char *outdata, int outlen, char** retdata, int *retlen);

  /** 1 way RPC. Ie a remote call without a reply.
      @param outdata outgoing data
      @param outlen outgoing data size
      @return OK or FAILED
  */
  int call1way(char *outdata, int outlen);

  /** Set the timeout time waiting for a reply from an RPC.
      @param timeout is either QUANTAnet_rpcClient_c::NEVER_TIME_OUT or time in seconds.
  */
  void setTimeOut(int timeout = QUANTAnet_rpcCaller_c::NEVER_TIME_OUT) {itsTimeOut = timeout;}

  QUANTAnet_rpcCaller_c();

  //for performance monitoring

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
  char *rpcName;
  char ipAddress[256];
  int destinationPort;
  int itsTimeOut;
  //For performance monitoring
  QUANTAnet_perfMonitor_c pmonitor;
};


#endif
