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

#ifndef _QUANTAPLUS_HTTP
#define _QUANTAPLUS_HTTP

#include <QUANTA/QUANTAnet_tcp_c.hxx>
#ifndef _WIN32_WCE
#include <sys/types.h>
#endif
#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif

#define INBUFFER_CONST 2048
#define BUFFER_CONST 1024

class QUANTAnet_perfMonitor_c;
/** A class to load files from WEB servers.
 *
 * This is a class to grab a file off a WEB site depending on
 * whether the local cached version is older than the remote one.

 * Begin by creating a webget object. Using the accessor functions,
 * assign to it the remote WEB address,
 * path and file. Then assign the local path and file.

 * Call SetMode to choose the operation you wish.

 * Finally call GrabWEBFile and check the return status.
 *
 * @author (C) 1997 Jason Leigh.
 * @version 4/1/97.
 */

class QUANTAnet_http_c {

public:

  /// SetMode flags.
  //@{
  /// Autoload from WEB page checking local cached file.
  static const int AUTO_LOAD/* = 0*/;
  /// Check the status of WEB page.
  static const int CHECK_BUT_DONT_LOAD/* = 3*/;
  /// Force a load from the WEB page ignoring the cache.
  static const int FORCED_LOAD/* = 1*/;
  //@}

  /// Turn timeout off when loading from WEB page.
  static const int NO_TIME_OUT/* = -1*/;

  QUANTAnet_http_c(){
    itsMode = AUTO_LOAD;
    timeOutPeriod =NO_TIME_OUT;
  }
  virtual ~QUANTAnet_http_c() {}

  /// Set the timeout period during WEB page loading.
  void setTimeOut(int timep) {timeOutPeriod = timep;}

  /// Accessor functions.
  //@{
  /// Set the IP address of remote site.
  void setRemoteSite(char *site);
  /// Set the path from the remote site.
  void setRemotePath(char *path);
  /// Set the remote file to download.
  void setRemoteFile(char *file);

  /// Set the local path to store downloaded file.
  void setLocalPath(char *path);
  /// Set the local file to store downloaded file.
  void setLocalFile(char *file);

  /** Set download mode.
      @param mode Set to either AUTO LOAD, FORCED LOAD or
      CHECK BUT DONT LOAD.
      @see SetMode Flags.
  */
  void setMode(int mode) {itsMode = mode;}

  /// Get timeout period.
  int getTimeOut() {return timeOutPeriod;}

  /// Counterpart of SetRemoteSite.
  char* getRemoteSite() {return remoteSite;}
  /// Counterpart of SetRemotePath.
  char* getRemotePath() {return remotePath;}
  /// Counterpart of SetRemoteFile.
  char* getRemoteFile() {return remoteFile;}

  /// Counterpart of SetLocalPath.
  char* getLocalPath() {return localPath;}

  /// Counterpart of GetLocalFile.
  char* getLocalFile() {return localFile;}

  /// Counterpart of SetMode.
  int getMode() { return itsMode;}
  //@}

  // site = WEB IP address
  // path = path to where to grab the file (no leading or trailing / needed)
  // filename = name of file to grab.
  // localPath = local path to store the file (no trailing / needed)
  // localFile = local filename
  // Set timeout to -1 for no timeout else timeout is in seconds

  /** Grab the WEB file.
   * Mode of grab is determined by SetMode method.
   * @return 200 if modified and downloaded ok.
   * -1 if timed out but cache file exists.
   * -11 if timed out but cache file DOES NOT exist.
   * -2 if cannot connect.
   * -3 if get of file successful but rename of tmpfile failed.
   * -4 if unable to write to local file.
   * -5 if modified : only for CHECK_BUT_DONT_LOAD mode.
   * -6 if unable to get the content length of the remote file.
   * else returns WEB status numbers.
   * e.g. 304 is file not modified.
   * 404 is file not found.
   */
  int grabWEBFile();

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
     Properly delete the perFDaemon Client after sendStats is done
  */
  void exitSendStats();



private:
  //  static const int OUTBUFFER = 512;
  static const int INBUFFER/* = 2048*/;
#ifndef WIN32
  // Windows provides a macro function min already...
  int min(int a, int b) {
    if (a < b) return a; else return b;
  }
#endif
  int timeDateOfFile(char* filename, char *out,
                     int *date,
                     int *month,
                     int *year,
                     int *hour,
                     int *minute,
                     int *second);

  char sendBuffer[INBUFFER_CONST];
  char receiveBuffer[INBUFFER_CONST];

  static const int BUFFER/* = 1024*/;

  char remoteSite[BUFFER_CONST];
  char remotePath[BUFFER_CONST];
  char remoteFile[BUFFER_CONST];
  char localPath[BUFFER_CONST];
  char localFile[BUFFER_CONST];
  int timeOutPeriod;
  int itsMode;

  //For performance monitoring calculations
  QUANTAnet_perfMonitor_c pmonitor;

  // Search for a string in an array of strings and return the index count.
  // Return -1 if not found.
  int indexIntoString(char **strlist, int len, char *searchstr);

  // If date 1 < date 2 return -1.
  // If date 1 > date 2 return 1.
  // If date 1 == date 2 return 0.
  int compareDate(int date1, int month1, int year1,
                  int hour1, int minute1, int second1,
                  int date2, int month2, int year2,
                  int hour2, int minute2, int second2);

  // Given a HTTP 1.0 Date string convert it into component values.
  int webDateConvert(char *instring,
                     int *date,
                     int *month,
                     int *year,
                     int *hour,
                     int *minute,
                     int *second);

};
#endif
