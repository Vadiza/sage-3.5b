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

#ifndef _QUANTA_BARRIER_UDP_H
#define _QUANTA_BARRIER_UDP_H

#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_barrierBase_c.hxx>


/**
 * Barrier Udp class
 */

class QUANTAnet_barrierUdp_c : public QUANTAnet_barrierBase_c
{
public:
  QUANTAnet_barrierUdp_c(char *key_id, int block_count=1, unsigned int port=0, int verbose=0);
  virtual ~QUANTAnet_barrierUdp_c(void);

protected:

private:
  QUANTAnet_udp_c *_udpSocket;
  //    QUANTAnet_udp_c *_nodes[MAX_NODES]; // array containing waiting nodes
  //    int _node_count;


  /**
   * all nodes have send a completion confirmation, tell all nodes to carry on
   **/
  virtual void broadcast(void);


  /**
   * Check to see if the current node
   * is one that we have not encountered before
   **/
  virtual void checkNode(void);


  /**
   * check if the curent node is valid - the current node has the same
   * id as this barrier
   * @return validation status
   **/
  virtual int validateNode(void);


  /**
   * check for special command sent from a node
   **/
  virtual void checkSpecialCommands(void);


  /**
   * the main loop that processes nodes
   **/
  void run(void);

  /**
   * remove a node from the barrier
   * @param obj node to remove
   **/
  void removeNode(void *obj);

};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Barrier Node Udp class
 */

class QUANTAnet_barrierNodeUdp_c : public QUANTAnet_barrierNodeBase_c
{
public:
  QUANTAnet_barrierNodeUdp_c(char *host = NULL, unsigned int port = 0);
  ~QUANTAnet_barrierNodeUdp_c(void);

  /**
   * stop this node's processing until the barrier releases it
   * @param key_id identification string to send to barrier
   **/
  void wait(char *key_id);


  /**
   *
   **/
  void setBarrierBlockCount(int block_count);


  /**
   *
   **/
  void setBarrierWaitCount(int wait_count);


  /**
   *
   **/
  void setBarrierKeyID(char *key_id);


protected:

private:
  QUANTAnet_udp_c *_mainUdpSocket;

  void sendError(char *params);
  void sendCommand(char *command, int arg);
  void sendCommand(char *command, float arg);
  void sendCommand(char *command, char *arg);

};


#endif
