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

#ifndef _QUANTAPLUS_RBUDPBASE_C
#define _QUANTAPLUS_RBUDPBASE_C

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>  // for iovec{} and readv/writev
#include <sys/stat.h>
#include <sys/mman.h>


#include <strings.h>

#include <QUANTA/QUANTAglobals.hxx>

#define USEC(st, fi) (((fi)->tv_sec-(st)->tv_sec)*1000000+((fi)->tv_usec-(st)->tv_usec))

struct _rbudpHeader
{
  int seq;
};
struct _endOfUdp
{
  int round;
  char end[3];
};

typedef struct _QUANTA_iovec
{
  void *iov_base;
  int iov_len;
} QUANTA_iovec;


/** RBUDP base class.  This enables us to transfer a block of memory over UDP reliably.  This base class cannot be instantiated.

    TCP needs fine tune over long fat pipes.  But UDP can get the maximum
    performance.  With RBUDP, we can get both maximum performance and reliable transfer. RBUDP try to keep the network pipe as full as possible during the bulk data
    transfer by avoiding the slow start session and congestion control mechanism
    of TCP protocol.  Usually the throughput of RBUDP can overperform single
    stream TCP or even parallel TCP, especially over long, fat pipes.
    If you are writing a sender you need to instantiate a
    QUANTAnet_rbudpSender_c object. Likewise if you are writing a Receiver you
    need to instantiate a QUANTAnet_rbudpReceiver_c object.

*/


class QUANTAnet_rbudpBase_c
{
protected:
  char * mainBuffer;
  int dataSize;
  int sendRate;
  int payloadSize;
  int headerSize;
  int packetSize;
  int totalNumberOfPackets;
  int remainNumberOfPackets;
  int lastPayloadSize;
  int usecsPerPacket;

  // RBUDP will reuse the existing tcp socket if it is true.
  int hasTcpSock;

  // UDP socket used in RBUDP data channel
  int udpSockfd;

  // TCP listen socket
  int listenfd;

  // TCP socket used in RBUDP control channel
  int tcpSockfd;

  // TCP server port
  int tcpPort;

  // UDP local port
  int udpLocalPort;

  // UDP remote port
  int udpRemotePort;

  // Size of the file name the receiver wants to get from the sender
  static const int SIZEOFFILENAME = 256;

  static const int SUCCESS = 1;

  static const int FAILED = -1;

  struct sockaddr_in udpServerAddr;
  long * hashTable;
  char * errorBitmap;
  int sizeofErrorBitmap;
  int isFirstBlast;
  FILE *log;
  struct _endOfUdp endOfUdp;

  QUANTA_iovec * ioVec;
  int iovecCount;

  int * packetCount;
  int * lastPloadSize;

  int reportTime(struct timeval & curTime);

protected:
  /// Connect to a remote UDP server with the default UDP port
  void connectUDP(char *host);
  /// Establish a UDP server at the default UDP port
  void passiveUDP(char *host);
  /// Connect to a remote TCP server with the default TCP port
  int connectTCP(char *host);
  /// Initialize a TCP server at the default TCP port
  void initTCPServer();
  /// TCP server starts listening
  void listenTCPServer();
  /// Read nbytes bytes from TCP connection
  int readn(register int fd, register char *ptr, register int nbytes);
  /// Write nbytes bytes to TCP connection
  int writen(register int fd, register char *ptr, register int nbytes);
  /// Initialize the bitmap
  void initErrorBitmap();
  /// Update the bitmap. Should be called every time when a UDP packet is received
  void updateErrorBitmap(long seq);
  /// Update the hash table of need-to-send UDP packets.  Should be called when a new bitmap is received.:w
  int updateHashTable();
  /// return the pointer according to the iovec data
  void * getActualPointer(int seqno);
  /// return the actual length of the packet
  int lengthPacket(int seqno);
};

#endif
