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

#ifndef _QUANTA_NET_OBJECT_STREAM_TCP_H
#define _QUANTA_NET_OBJECT_STREAM_TCP_H

#include <QUANTA/QUANTAnet_objectStreamBase_c.hxx>


//
// this class is a wrapper for the QUANTAnet_objectStreamBaseTcp_c class
// it does nothing other than provide a public constructor, which allows
// access to the QUANTAnet_objectStreamBaseTcp_c functions
//

/**
 * ObjectOutputStream Tcp class
 */

class QUANTAnet_objectOutputStreamTcp_c : public QUANTAnet_objectStreamBaseTcp_c
{
public:
  QUANTAnet_objectOutputStreamTcp_c(char *host, int port);
  ~QUANTAnet_objectOutputStreamTcp_c(void);


private:


};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * ObjectInputStream Tcp class
 */

class QUANTAnet_objectInputStreamTcp_c : public QUANTAnet_objectStreamBaseTcp_c
{
public:
  QUANTAnet_objectInputStreamTcp_c(int port);
  QUANTAnet_objectInputStreamTcp_c(void);
  ~QUANTAnet_objectInputStreamTcp_c(void);


  /**
   * get the network client
   *@return the network client
   **/
  QUANTAnet_tcpServer_c *getNetworkServer(void);


  /**
   * set the network client
   *@param client the network client
   **/
  void setNetworkServer(QUANTAnet_tcpServer_c *server);


  /**
   * close the socket
   **/
  void close(void);


protected:
  QUANTAnet_tcpServer_c *_server;

};


#endif
