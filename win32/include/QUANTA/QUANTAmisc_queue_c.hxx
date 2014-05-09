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

#ifndef _QUANTA_MISC_QUEUE_H
#define _QUANTA_MISC_QUEUE_H

#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif
#include <stdlib.h>
#include <QUANTA/QUANTAmisc_exception_c.hxx>
#include <QUANTA/QUANTAglobals.hxx>


/**
 * sinlge ended queue class.
 * objects are added to the rear and leave from the front (first in, first out).
 **/

template <class T>
class QUANTAmisc_queue_c;

/**
 * The <CODE>QUANTAmisc_queueNode_c</CODE> class is used as a storage object for data.
 * It further keeps track of the next QUANTAmisc_queueNode_c in the sequence with the
 * use of <CODE>next</CODE> QUANTAmisc_queueNode_c data member.
 * For internal use only by QUANTAmisc_queue_c class.
 **/
template <class T>
class QUANTAmisc_queueNode_c
{
  friend class QUANTAmisc_queue_c<T>;

public:
  /**
   * all nodes are deleted by the QUANTAmisc_queue_c class
   **/
  ~QUANTAmisc_queueNode_c(void);


private:
  /**
   * Creates a new, completely empty QUANTAmisc_queueNode_c.  The data and next
   * references are all set to <CODE>NULL</CODE>.
   **/
  QUANTAmisc_queueNode_c(void);


  /**
   * Creates a new QUANTAmisc_queueNode_c which contains the specified data.
   * The next reference is set to <CODE>NULL</CODE>.
   *
   * @param data the data being stored in this QUANTAmisc_queueNode_c
   **/
  QUANTAmisc_queueNode_c(const T &data);


  /**
   * Creates a new QUANTAmisc_queueNode_c which contains the specified data with
   * specified next references.
   *
   * @param data the data being stored in this QUANTAmisc_queueNode_c
   * @param next the node to follow this QUANTAmisc_queueNode_c
   **/
  QUANTAmisc_queueNode_c(const T &data, QUANTAmisc_queueNode_c<T> *next);


  /**
   * The data this node is keeping track of.
   **/
  T _data;


  /**
   * The node following this QUANTAmisc_queueNode_c.  If there is no such node,
   * <CODE>next</CODE> should have the value <CODE>NULL</CODE>.
   **/
  QUANTAmisc_queueNode_c<T> *_next;
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_queueNode_c<T>::QUANTAmisc_queueNode_c(void)
{
  _next = NULL;
}


template <class T>
QUANTAmisc_queueNode_c<T>::QUANTAmisc_queueNode_c(const T &data)
  : _data(data)
{
  _next = NULL;
}


template <class T>
QUANTAmisc_queueNode_c<T>::QUANTAmisc_queueNode_c(const T &data, QUANTAmisc_queueNode_c<T> *next)
  : _data(data), _next(next)
{
}


template <class T>
QUANTAmisc_queueNode_c<T>::~QUANTAmisc_queueNode_c(void)
{
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <class T>
class QUANTAmisc_queue_c
{
public:
  /**
   * Creates a new, initially empty QUANTAmisc_queue_c.  The current node is undefined.
   **/
  QUANTAmisc_queue_c(void);


  /**
   * Destroy a QUANTAmisc_queue_c.
   **/
  ~QUANTAmisc_queue_c(void);


  /**
   * Returns the number of items currently in this QUANTAmisc_queue_c.
   *
   * @return the number of items currently in this QUANTAmisc_queue_c
   **/
  inline int size(void);


  /*******
   * Checks if the QUANTAmisc_queue_c is empty.
   *
   * @return true if the QUANTAmisc_queue_c is empty; otherwise, returns false.
   *******/
  inline bool isEmpty(void);


  /*****
   * Inserts an object at the rear end of the queue.
   *
   * @param data an object to add to the queue
   ****/
  void enqueue(const T &data);


  /******
   * Removes and returns the object at the front of the queue.
   *
   * @return the object at the front of the queue
   * @exception QUANTAmisc_emptyQueueException_c, throw if the queue is empty
   ******/
  T &dequeue(void) throw(QUANTAmisc_emptyQueueException_c);

  /**
   * Removes all nodes and resets this QUANTAmisc_queue_c to default values (same as an
   * empty QUANTAmisc_queue_c created with the constructor). If there are no items in this
   * QUANTAmisc_queue_c, an exception is issued.
   *
   * @exception NoCurrentException if there are no nodes in this QUANTAmisc_queue_c
   **/
  void clear(void); //throw(QUANTAmisc_emptyQueueException_c);


private:
  // number of nodes in the sequence
  int _size;

  QUANTAmisc_queueNode_c<T> *_head_node;
  QUANTAmisc_queueNode_c<T> *_tail_node;
  QUANTAmisc_queueNode_c<T> *_temp_node; // used to create new item in the sequence
  T _temp_data; // used to return node data
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_queue_c<T>::QUANTAmisc_queue_c(void)
{
  // set the node count to zero
  _size = 0;

  // create a new node with data, next, and prev all NULL
  _head_node = new QUANTAmisc_queueNode_c<T>();

  // set the current and tail nodes to the head node,
  // which contains NULLs for data, next, and prev
  _tail_node = _head_node;
  _temp_node = NULL;
}


template <class T>
QUANTAmisc_queue_c<T>::~QUANTAmisc_queue_c(void)
{
  try {
    clear();
  } catch(QUANTAmisc_exception_c ex) {}
  // no need to delete _tail_node.
  // _tail_node points to _head_node after removeAll() is called
  delete _head_node;
  delete _temp_node;
}


template <class T>
inline int QUANTAmisc_queue_c<T>::size(void)
{
  return _size;
}


template <class T>
inline bool QUANTAmisc_queue_c<T>::isEmpty(void)
{
  if(_size == 0 || _head_node == NULL)
    return true;

  return false;
}


template <class T>
void QUANTAmisc_queue_c<T>::enqueue(const T &data)
{
  // if tail node is NULL (queue is empty) set tail node to head node
  if(_tail_node == NULL)
  {
    _head_node = new QUANTAmisc_queueNode_c<T>();
    _tail_node = _head_node;
  }
  // create a new node to add to the queue; tail.next should always be NULL
  _temp_node = new QUANTAmisc_queueNode_c<T>(data, _tail_node->_next);

  // set the tail node's next pointer to the temp node
  _tail_node->_next = _temp_node;
  // advance the tail pointer to the temp node
  _tail_node = _temp_node;

  // increment the size counter; added a node to the queue
  _size++;
  // set temp to NULL
  _temp_node = NULL;
}


template <class T>
T &QUANTAmisc_queue_c<T>::dequeue(void) throw(QUANTAmisc_emptyQueueException_c)
{
  // if the queue is empty throw an exception
  if(_size == 0 || _head_node == NULL)
    throw QUANTAmisc_emptyQueueException_c("QUANTAmisc_queue_c::getCurrent() - no current nodes.");

  _temp_data = _head_node->_next->_data; // this node's data to return
  // assign head node to a temp node so that this node can be deleted
  _temp_node = _head_node;
  _head_node = _head_node->_next;
  if(_head_node == NULL)
  {
    _tail_node = _head_node;
  }

  /*
  // if current's next node is NULL (the current node is the tail node),
  // assign the new current node to current's previous node, and re-assign
  // the tail pointer to the new current pointer
  if(_head_node->_next == NULL)
  {
  _tail_node = _head_node;
  }
  else
  {
  // current's next node is another valid node, so re-assign current's
  // next's node's previous pointer to the node before current
  // and re-assign the current pointer to current's next node
  _head_node = _head_node->_next;
  }
  */
  // decrement this sequence's node counter
  _size--;

  // trash the node that was dropped from the queue
  delete _temp_node;
  _temp_node = NULL;

  // return the original head node's data
  return _temp_data;
}


template <class T>
void QUANTAmisc_queue_c<T>::clear(void) //throw(QUANTAmisc_emptyQueueException_c)
{
  //  if(_size == 0 || _head_node == NULL)
  //    throw QUANTAmisc_emptyQueueException_c("QUANTAmisc_queue_c::getCurrent() - no current nodes.");

  while(_head_node->_next != NULL)
  {
    _temp_node = _head_node; // assign node to delete
    _head_node = _head_node->_next;
    delete _temp_node;
    _size--;
  }

  _tail_node = _head_node = NULL;
  _temp_node = NULL;
}


#endif
