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

#ifndef _QUANTA_NET_OBJECT_STREAM_BASE_H
#define _QUANTA_NET_OBJECT_STREAM_BASE_H

#include <string.h>
#include <stdlib.h>

#include <QUANTA/QUANTAnet_tcp_c.hxx>
#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_serializable_c.hxx>

/**
 * ObjectStream Base class
 */

class QUANTAnet_objectStreamBase_c
{
public:
  virtual ~QUANTAnet_objectStreamBase_c(void);


  /**
   * read an object from a stream
   * @param obj object to read
   * @return status
   **/
  virtual int readObject(QUANTAnet_serializable_c *obj) = 0;


  /**
   * write an object to a stream
   * @param obj object to write
   * @return status
   **/
  virtual int writeObject(QUANTAnet_serializable_c *obj) = 0;


  /**
   * close the socket
   **/
  virtual void close(void) = 0;


protected:
  QUANTAnet_objectStreamBase_c(char *host=NULL, int port=0);

  char *_host;
  int _port;
  char *_send_buffer;
  char *_receive_buffer;

};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * ObjectStream Base Tcp class
 */

class QUANTAnet_objectStreamBaseTcp_c : public QUANTAnet_objectStreamBase_c
{
public:
  virtual ~QUANTAnet_objectStreamBaseTcp_c(void);

  /**
   * read an object from a stream
   * @param obj object to read
   * @return status
   **/
  virtual int readObject(QUANTAnet_serializable_c *obj);


  /**
   * write an object to a stream
   * @param obj object to write
   * @return status
   **/
  virtual int writeObject(QUANTAnet_serializable_c *obj);


  /**
   * close the socket
   **/
  virtual void close(void);


  /**
   * get the network client
   *@return the network client
   **/
  QUANTAnet_tcpClient_c *getNetworkClient(void);


  /**
   * set the network client
   *@param client the network client
   **/
  void setNetworkClient(QUANTAnet_tcpClient_c *client);


protected:
  QUANTAnet_objectStreamBaseTcp_c(char *host, int port);
  QUANTAnet_objectStreamBaseTcp_c(void);

  QUANTAnet_tcpClient_c *_client;


private:

};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * ObjectStream Base Udp class
 */

class QUANTAnet_objectStreamBaseUdp_c : public QUANTAnet_objectStreamBase_c
{
public:
  virtual ~QUANTAnet_objectStreamBaseUdp_c(void);

  /**
   * read an object from a stream
   * @param obj object to read
   * @return status
   **/
  virtual int readObject(QUANTAnet_serializable_c *obj);


  /**
   * write an object to a stream
   * @param obj object to write
   * @return status
   **/
  virtual int writeObject(QUANTAnet_serializable_c *obj);


  /**
   * close the socket
   **/
  virtual void close(void);


  /**
   * get the network client
   *@return the network client
   **/
  QUANTAnet_udp_c *getNetworkClient(void);


  /**
   * set the network client
   *@param client the network client
   **/
  void setNetworkClient(QUANTAnet_udp_c *client);


protected:
  QUANTAnet_objectStreamBaseUdp_c(char *host, int port);
  QUANTAnet_objectStreamBaseUdp_c(void);

  QUANTAnet_udp_c *_client;


private:

};


#endif
