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

#ifndef _QUANTANET_FECCLIENT_C
#define _QUANTANET_FECCLIENT_C

#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_fecEngine_c.hxx>

typedef struct {
  unsigned short   fecPackNum;    //  FEC pack sequence No. in the FEC sub-block
  unsigned short   BlockNum;   //  FEC block sequence No.
  unsigned int   dataSize;      //  size of data
  char  *data;            //  data
} FECPack;

typedef struct {
  unsigned short   srcPackNum;  // Source pack sequence No. in the FEC block
  unsigned short   BlockNum;          //  FEC block sequence No.
  unsigned int   dataSize;  // size of data
  char  *data;      // data
} SourcePack;

#define USEC(st, fi) (((fi)->tv_sec-(st)->tv_sec)*1000000+((fi)->tv_usec-(st)->tv_usec))

/**
   FEC Client Class.  This class implements the client side of FEC over UDP protocol.
   First, instantiate the QUANTAnet_fecClient_c class. Then call this object's init()
   method with parameters such as FEC ratio, packet size, server host name, source port,
   FEC port.  Then you can call this object's streamingSend to streaming date to the
   server.  Before that you need to create your own update funstion to update the send
   buffer.

*/

class QUANTAnet_fecClient_c {

public:

  QUANTAnet_fecClient_c();

  virtual ~QUANTAnet_fecClient_c();

  /// Get number of source pack in the whole block.
  unsigned int getNumSourcePack();

  /// Get number of packets in the whole block.
  unsigned int getNumTotalPack();

  /// Get the size of payload.
  unsigned int getPayloadSize();

  /// Get QUANTAnet_udp_c pointer to the source packet socket.
  QUANTAnet_udp_c* getSrcUDPSocket();

  /// Get QUANTAnet_udp_c pointer to the FEC packet socket.
  QUANTAnet_udp_c* getFecUDPSocket();

  /// Set the sending rate at Mbps.
  void setSendRate(double rate);

  /// Get the sending rate at Mbps.
  double getSendRate();

  /* Initialize. The following parameters are needed: numSourcePack:number of source pack in the
   * block, numTotalPack:number of packets in the block, payloadSize:size of payload,
   * host:server's name, srcPort:source port number, fecPort:FEC port number
   */
  void init(unsigned int numSourcePack, unsigned int numTotalPack,
            unsigned int payloadSize, char* host, unsigned short srcPort, unsigned short fecPort);

  /// Send the content pointed by data.
  int send(char* data);

  void senddata(char* data, int size, int rate);

  /// Streaming data updated by updateFunc.
  void streamingSend(void(*updateFunc)(void*, int));

private:

  /// Set number of source pack in the whole block
  void setNumSourcePack(unsigned int numSourcePack);

  /// Set number of packets in the whole block
  void setNumTotalPack(unsigned int numTotalPack);

  /// Set the size of payload
  void setPayloadSize(unsigned int payloadSize);

  /// Set hostname and port number for QUANTAnet_udp_c object of source packet
  int setSrcSendAddress(char* host, unsigned short port);

  /// Set hostname and port number for QUANTAnet_udp_c object of FEC packet
  int setFecSendAddress(char* host, unsigned short port);

private:

  unsigned short _countBlock;   // counter of packets in the whole block
  unsigned short _countSourcePack;  // counter of source packets

  unsigned short _countFecPack;   // counter of FEC packets the FEC sub-block
  unsigned short _countFecBlock;    // counter of FEC sub-blocks

  unsigned int _numSourcePack;    // number of source packets in the whole block
  unsigned int _numTotalPack;   // number of packets in the whole block
  unsigned int _payloadSize;    // size of payload

  int offsite;        // SourcePack/FECPack offsite from the beginning to the data(payload)
  int len;        // The total size of SourcePack/FECPack(including header and payload)

  double _sendRate; // raw data sending rate, the real sending rate is higher than this.

  fec_parms *fecMatrix;     // Parameters for FEC codec

  QUANTAnet_udp_c *srcUDPSocket;    // QUANTAnet_udp_c pointer for source packets
  QUANTAnet_udp_c *fecUDPSocket;    // QUANTAnet_udp_c pointer for FEC packets
  QUANTAnet_fecEngine_c *fecEngine; // QUANTAnet_fecEngine_c

  char **srcData;       // Buffer of source packets in the whole block
  char **totalData;     // Buffer of total packets in the whole block

  char *srcPackPool;      // Sending buffer of source packets
  char *fecPackPool;      // Sending buffer of FEC packets

  SourcePack srcPack;
  FECPack fecPack;

};
#endif /* QUANTAnet_fecClient_c */
