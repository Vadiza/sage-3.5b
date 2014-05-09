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

/* File: QUANTAnet_fecServer_c.hxx
   Description: The header file for QUANTAnet_fecServer_c class
*/

#ifndef _QUANTANET_FECSERVER_C
#define _QUANTANET_FECSERVER_C

#include <sys/time.h>

#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_fecEngine_c.hxx>


/*enum PACKET_STATUS{ PACKET_LOSS=0,PACKET_OK,PACKET_RECOVERED,PACKET_NOT_ARRIVED,
  PACKET_NO_DATA,PACKET_ERROR};*/

#define PACKET_LOSS 0
#define PACKET_OK 1
#define PACKET_RECOVERED 2
#define PACKET_NOT_ARRIVED 3
#define PACKET_NO_DATA 4
#define PACKET_ERROR 5


typedef struct {
  unsigned short   fecPackNum;    //  FEC pack sequence No. in the FEC sub-block
  unsigned short   BlockNum;   //  FEC block sequence No.
  unsigned int   dataSize;      //  size of data
  char  *data;            //  data
} FECPack;

typedef struct {
  unsigned short   srcPackNum;  // Source pack sequence No. in the block
  unsigned short   BlockNum;          //  FEC block sequence No.
  unsigned int   dataSize;  // size of data
  char  *data;      // data
} SourcePack;


/**
   FEC Server Class.  This class implements the server side of FEC over UDP protocol.
   First, instantiate the QUANTAnet_fecServer_c class. Then call this object's init()
   method with parameters such as FEC ratio, packet size, source port, FEC port, etc.
   Then you can call this object's streamingRecv to receive streaming data from the
   client.  Before that you need to create your own data export funstion to export
   the received buffer.

*/

class QUANTAnet_fecServer_c {

public:
  QUANTAnet_fecServer_c();

  virtual ~QUANTAnet_fecServer_c();

  /// Get number of source pack in the whole block
  unsigned int getNumSourcePack();

  /// Get number of packets in the whole block
  unsigned int getNumTotalPack();

  /// Get the size of payload
  unsigned int getPayloadSize();

  /// Get number of rows of the receiving buffer
  unsigned short getNumRowofRecvBuff();

  /// Get the timeout value. Timeout value is the max time for waiting a source packet coming.
  unsigned short getTimeOut();

  /// Get QUANTAnet_udp_c pointer to the source packet socket
  QUANTAnet_udp_c* getSrcUDPSocket();

  /// Get QUANTAnet_udp_c pointer to the FEC packet socket
  QUANTAnet_udp_c* getFecUDPSocket();

  /* Initialize. The following parameters are needed: numSourcePack:number of source pack in the
   * block, numTotalPack:number of packets in the block, payloadSize:size of payload,
   * numRowofRecvBuff: number of rows of receiving buffer, timeout:timeout value,
   * srcPort:source port number, fecPort:FEC port number
   */
  void init(unsigned int numSourcePack, unsigned int numTotalPack,
            unsigned int payloadSize, unsigned short numRowofRecvBuff, unsigned
            short timeOut, unsigned short srcPort, unsigned short fecPort);

  /// Receive data.
  int receive(char* message);

  /// Receive streaming data.  Export the data using exportFunc.
  void streamingRecv(void(*exportFunc)(void*, int));

  /// Print statistics data.
  void printStatisticsData();

private:

  /// Set number of source pack in the whole block
  void setNumSourcePack(unsigned int numSourcePack);

  /// Set number of packets in the whole block
  void setNumTotalPack(unsigned int numTotalPack);

  /// Set the size of payload
  void setPayloadSize(unsigned int payloadSize);

  /// Set number of rows of the receiving buffer
  void setNumRowofRecvBuff(unsigned short numRowofRecvBuff);

  /// Set port number for QUANTAnet_udp_c object of source packet
  int setSrcReceivePort(unsigned short port);

  /// Set port number for QUANTAnet_udp_c object of FEC packet
  int setFecReceivePort(unsigned short port);

  /// Set timeout value
  void setTimeOut(unsigned short timeOut);

  /// Memory allocation/free functions
  char*** my2dNew(int x, int y, int size);
  char** myNew(int x, int size);
  void my2dDelete(char*** data, int x, int y);
  void myDelete(char** data, int x);

  /// Check if the lost source packet can be recovered.
  int isRecoverReady();

  /// Set RecvFlag for a packet. 0-Not Arrived, 1-Arrived, 2-Recovered
  void setRecvFlag(int x, int y, int value);

  /// Clear RecvFlag
  void clearRecvFlag();

  /// Clear RecvFlag in the current row
  void clearCurrentRowRecvFlag();

  /// Get RecvFlag for a certain packet
  unsigned short getRecvFlag(int x, int y);

  /// Check how many packets are received and put them in the right order
  void calcRecvPack();

  /// Put the recovered source packets back into the receiving buffer
  void putBackRecoveredPack();

  /// Construct a SourcePack from received message
  void packSrc(char* message);

  /// Construct a FECPack from received message
  void packFec(char* message);

  /// Check if the expected packet is in the receiving buffer
  int isInRecvBuff(unsigned short seqNo);

  /// Store the received source packet in the receiving buffer with the right position
  void storeSrcPack(SourcePack* pack);

  /// Store the received FEC packet in the receiving buffer with the right position
  void storeFecPack(FECPack* pack);

  /// Check source packet arriving status
  int checkArriveStatus();

  /// Check FEC packet arriving status
  void checkFecArriveStatus();

  /// Set a timer for timeout
  void setAlarm();

  /// Clear the timer
  void clearAlarm();

  /// Check if the timer is set
  int isAlarmEnabled();

  /// Check if it's timeout
  int isTimeOut();

  /// Calculation utilities
  void positionTranslate(unsigned short* x, unsigned short* y, unsigned
                         short num);
  void nextCurrentRowIndex();
  void nextExpectNextSrcPackNum();
  void calcMaxBlockNum();
  void calcRecvSrcBuffSize();

  int checkAllSrcDataRecv();
  int countLostPackets();
  void packetRecvHandler(void(*exportFunc)(void*, int));

private:

  unsigned int _numSourcePack;    // number of source packets in the whole block
  unsigned int _numTotalPack;   // number of packets in the whole block
  unsigned int _payloadSize;    // size of payload
  unsigned short _numRowofRecvBuff; // number of rows of the receiving buffer on the server
  unsigned int _recvSrcBuffSize;    // source packet receiving buffer size
  unsigned int _recvFecBuffSize;    // FEC packet receiving buffer size
  unsigned short _maxBlockNum;    // Max block number

  unsigned short pickupX;     // Row index of receiving buffer
  unsigned short pickupY;     // Column index of receiving buffer

  unsigned short currentRowIndex;   // Current row index of receiving buffer

  unsigned short expectNextSrcPackNum;  // Expected sequence number of next source packet

  int _countFinishedBlock; // the count of finished blocks
  int _countSrcPack;    // the count of current source packet
  int _countFecPack;
  int _countFecBlock;  // the count of current FEC block

  int _countRecoveredBlock;  // statistics data
  int _countFailedBlock;    // statistics data
  int _countFailedPacket;

  FILE *logFilePtr;

  unsigned short _timeOut;    // Timeout
  unsigned short timeOutFlag;   // Timeout flag
  struct timeval startTime;   // Starting time
  double msStartTime;     // Time duration

  int offsite;        // SourcePack/FECPack offsite from the beginning to the data(payload)
  int len;        // The total size of SourcePack/FECPack(including header and payload)

  fec_parms *fecMatrix;     // Parameters for FEC codec

  QUANTAnet_udp_c *srcUDPSocket;    // QUANTAnet_udp_c pointer for source packets
  QUANTAnet_udp_c *fecUDPSocket;    // QUANTAnet_udp_c pointer for FEC packets
  QUANTAnet_fecEngine_c *fecEngine; // QUANTAnet_fecEngine_c

  char *srcPackPool;      // Receiving buffer of source packets
  char *fecPackPool;      // Receiving buffer of FEC packets

  SourcePack srcPack;
  FECPack fecPack;

  char **recvPack;      // Buffer for recovering packets
  int *recvIndex;       // Array of received packets' index
  unsigned short *recoverReady;   // Array of recoverReady flag
  char *** recvBuff;      // Receiving buffer
  unsigned short **recvFlag;    // Array of recvFlag


};
#endif /* QUANTAnet_fecServer_c */
