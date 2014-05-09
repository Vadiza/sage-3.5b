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

#ifndef _QUANTANET_PERFMONITOR_C
#define _QUANTANET_PERFMONITOR_C


#include <QUANTA/QUANTAnet_socketbase_c.hxx>
#include <QUANTA/QUANTAnet_perfDaemon_c.hxx>

class QUANTAnet_perfDaemonClient_c;


/**
   This class does the performance monitoring calculations for QUANTA.
   The calculations in the QUANTAnet_socketbase class are used by an instance
   of this class, included in other higher level modules (an instance of this class is not generally created by the user).

   Along with the API documentation, is also included, a note on the calculated parameters, the units of calculation and how they are calculated.

   As mentioned, in general, an instance of this class need not be created by the user. Instead the API's provided in the higher level classes (like the showStats, logStats and sendStats) should be directly invoked whenever network monitoring statistics is required. This is because the monitoring is done in all the networking classes automatically for every data flow over a network, using an instance of this class. The user can hence use the API's of the higher level classes as mentioned above, which would ensure statistics to be extracted whenever required.

   API calls should be made by the user as often as required to obtain the
   monitored statistics in the netlogger format ( <label,value> pairs).

   Currectly, the following capabilities have been provided with the performance monitoring: statistics can be displayed instantaneously, logged into a user specified logfile or sent to another remote daemon for further analysis
*/



class QUANTAnet_perfMonitor_c: public QUANTAnet_socketbase_c
{
public:

  ///Return values
  //@{
  ///Operation successful
  static const int OK; //=0

  ///Operation Failed
  static const int FAILED;  //=1
  //@}
  /// Default constructor allocates memory for IP strings.
  QUANTAnet_perfMonitor_c();
  /// Copy constructor creates a deep copy of another object.
  QUANTAnet_perfMonitor_c(const QUANTAnet_perfMonitor_c&);

  virtual ~QUANTAnet_perfMonitor_c();

  //set the source and destination IPs of the machines communicating over the network - not to be used by the user
  void setIPs(const char* selfIp, const char* remoteIp);

  //set the source and destination ports of the machines communicating over the network - not to be used by the user

  void setPorts(int selfPort, int remotePort);

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


  int initSendStats(const char* monitorClientIP, int port = QUANTAnet_perfDaemon_c::DEFAULT_PORT);

  /**
     Properly delete the perfDaemonClient after sendStats is done
  */

  void exitSendStats();

private:

  //source and destination IP's of the machines communicating over the network
  char *selfIp, *remoteIp;

  //source and destination ports of the machines communicating over the network

  int selfPort, remotePort;

  //The buffer in which the resultant stats is stored in netlogger format

  char* resultantStats;

  //A perfDaemonClient used to connect to a perfDaemon to send the resultant statistics

  QUANTAnet_perfDaemonClient_c* aClient;

  //Upate the performance statistics
  void updateStats(char* streamInfo, char* comment);


};




#endif
