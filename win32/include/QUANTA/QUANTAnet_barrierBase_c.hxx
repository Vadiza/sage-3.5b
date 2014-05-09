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

#ifndef _QUANTA_BARRIER_BASE_H
#define _QUANTA_BARRIER_BASE_H

#include <string.h>
#include <stdlib.h>

#include <QUANTA/QUANTAts_thread_c.hxx>
#include <QUANTA/QUANTAts_mutex_c.hxx>
#include <QUANTA/QUANTAmisc_list_c.hxx>
#include <QUANTA/QUANTAglobals.hxx>


#define MAX_LINE 256  // recieve line length
//#define MAX_NODES 256 // totlal number of nodes that this barrier can block for

/* tmp only - for tcp version */
/* This is the size of all messages sent from clients to servers. */
#define TRANSMITTED_DATA_SIZE 512
/* This is the size of all messages sent from servers to clients. */
#define BROADCAST_DATA_SIZE 1024


/**
 * Barrier base class
 */

class QUANTAnet_barrierBase_c : public QUANTAts_thread_c
{
public:
  virtual ~QUANTAnet_barrierBase_c(void);

  /**
   * set the id string that this barrier uses to make sure
   *  a node is syncing on the proper node group
   * @param key_id the string identification for this barrier
   **/
  inline void setKeyID(char *key_id) { _key_id = key_id; }


  /**
   * get the id string that this barrier uses to make sure
   *  a node is syncing on the proper node group
   * @return the string identification for this barrier
   **/
  inline const char *getKeyID(void) { return _key_id; }


  /**
   * set the total number of nodes this barrier will block for
   * @param block_count the number of nodes this barrier blocks for
   **/
  inline void setBlockCount(int block_count) { _block_count = block_count; }


  /**
   * get the total number of nodes this barrier will block for
   * @return the number of nodes this barrier blocks for
   **/
  inline int getBlockCount(void) { return _block_count; }


  /**
   * set the current number of nodes blocking on this barrier
   * @param node_count the current number of nodes this barrier is waiting on
   **/
  inline void setNodeCount(int node_count) { _wait_count = node_count; }


  /**
   * get the current number of nodes blocking on this barrier
   * @return the current number of nodes this barrier is waiting on
   **/
  inline int getNodeCount(void) { return _wait_count; }
  //    inline int getNodeCount(void) { return _nodes.size(); }


  /**
   * reset the number of nodes waiting on this barrier to zero
   **/
  inline void reset(void) { _wait_count = 0; }


  /**
   * return the port number this barrier is using
   * @return barrier port number
   **/
  inline unsigned int getPort(void) { return _port; }


  /**
   * turn on/off verbose mode, which outputs debug information to the stderr
   * @param verbose
   **/
  inline void setVerbose(int verbose) { _verbose = verbose; }


protected:
  QUANTAnet_barrierBase_c(char *key_id, int block_count=1, unsigned int port=0, int verbose=0);

  int _verbose; // if true, dump node info to stdout
  unsigned int _port;

  char *_key_id; // this barrier's id
  int _wait_count; // int _blocked_threads; number of nodes currently attached to this barrier; all block for the same key_id
  int _block_count; // int _barrier_val; this barrier will block/wait until this many node attach and send a completion confirmation

  QUANTAmisc_list_c<void *> _nodes; // collection of current connected nodes

  char _recvline[BROADCAST_DATA_SIZE];
  char command[MAX_LINE], arg[MAX_LINE];


  /**
   * all nodes have send a completion confirmation, tell all nodes to carry on
   **/
  virtual void broadcast(void) = 0;


  /**
   * Check to see if the current node
   * is one that we have not encountered before
   **/
  virtual void checkNode(void) = 0;


  /**
   * check if the curent node is valid - the current node has the same
   * id as this barrier
   * @return validation status
   **/
  virtual int validateNode(void) = 0;


  /**
   * check for special command sent from a node
   **/
  virtual void checkSpecialCommands(void) = 0;


private:

};


/******************************************************************************/
/**
 * BarrierNode base class
 */

class QUANTAnet_barrierNodeBase_c
{
public:
  virtual ~QUANTAnet_barrierNodeBase_c(void);

  virtual void wait(char *key_id) = 0;

protected:
  QUANTAnet_barrierNodeBase_c(char *host = NULL, unsigned int port = 0);
  char *_host;
  unsigned int _port;
  char _recvline[MAX_LINE];
  QUANTAts_mutex_c _mutex;

private:

};


#endif
