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

#ifndef _QUANTA_MISC_STACK_H
#define _QUANTA_MISC_STACK_H

#include <QUANTA/QUANTAmisc_exception_c.hxx>
#include <QUANTA/QUANTAglobals.hxx>


/**
 * QUANTAmisc_stack_c class.
 * objects enter and leave at the same end (first in, last out).
 **/

template <class T>
class QUANTAmisc_stack_c;

/**
 * The <CODE>QUANTAmisc_stackNode_c</CODE> class is used as a storage object for data.
 * It further keeps track of the next QUANTAmisc_stackNode_cs in the sequence
 * with the use of <CODE>next</CODE> QUANTAmisc_stackNode_c data
 * members. For internal use only by QUANTAmisc_stack_c class.
 **/
template <class T>
class QUANTAmisc_stackNode_c
{
  friend class QUANTAmisc_stack_c<T>;

public:
  /**
   * all nodes are deleted by the QUANTAmisc_stack_c class
   **/
  ~QUANTAmisc_stackNode_c(void);


private:
  /**
   * Creates a new, completely empty QUANTAmisc_stackNode_c.  The data, and next
   * references are all set to <CODE>NULL</CODE>.
   **/
  QUANTAmisc_stackNode_c(void);


  /**
   * Creates a new QUANTAmisc_stackNode_c which contains the specified data.
   * The next reference is set to <CODE>NULL</CODE>.
   *
   * @param data the data being stored in this QUANTAmisc_stackNode_c
   **/
  QUANTAmisc_stackNode_c(const T &data);


  /**
   * Creates a new QUANTAmisc_stackNode_c which contains the specified data with
   * specified next reference.
   *
   * @param data the data being stored in this QUANTAmisc_stackNode_c
   * @param next the node to follow this QUANTAmisc_stackNode_c
   **/
  QUANTAmisc_stackNode_c(const T &data, QUANTAmisc_stackNode_c<T> *next);


  /**
   * The data this node is keeping track of.
   **/
  T _data;


  /**
   * The node following this QUANTAmisc_stackNode_c.  If there is no such node,
   * <CODE>next</CODE> should have the value <CODE>NULL</CODE>.
   **/
  QUANTAmisc_stackNode_c<T> *_next;
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_stackNode_c<T>::QUANTAmisc_stackNode_c(void)
{
  _next = NULL;
}


template <class T>
QUANTAmisc_stackNode_c<T>::QUANTAmisc_stackNode_c(const T &data)
  : _data(data)
{
  _next = NULL;
}


template <class T>
QUANTAmisc_stackNode_c<T>::QUANTAmisc_stackNode_c(const T &data, QUANTAmisc_stackNode_c<T> *next)
  : _data(data), _next(next)
{
}


template <class T>
QUANTAmisc_stackNode_c<T>::~QUANTAmisc_stackNode_c(void)
{
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <class T>
class QUANTAmisc_stack_c
{
public:
  /**
   * Creates a new, initially empty QUANTAmisc_stack_c.  The top node is undefined.
   **/
  QUANTAmisc_stack_c(void);


  /**
   * Destroy this QUANTAmisc_stack_c.
   **/
  ~QUANTAmisc_stack_c(void);


  /*******
   * Pops the top item off the stack and returns it.
   *
   * @return the top item in the stack
   * @exception QUANTAmisc_emptyStackException_c if the stack is empty
   *******/
  T &pop(void) throw(QUANTAmisc_emptyStackException_c);


  /*******
   * Returns (but does not remove) the top item of the stack.
   *
   * @return the top item in the stack
   * @exception QUANTAmisc_emptyStackException_c if the stack is empty
   *******/
  inline T &peek(void) throw(QUANTAmisc_emptyStackException_c);


  /*******
   * Pushes the given item onto the top of the stack.
   *
   * @param obj the object to be pushed
   *******/
  void push(const T &data);


  /*******
   * Checks if the stack is empty.
   *
   * @return true if the stack is empty
   *******/
  inline bool isEmpty(void);


  /*******
   * Returns the size of the stack.
   *
   * @return the number of items on the stack
   *******/
  inline int size(void);


  /**
   * Removes all nodes and resets this QUANTAmisc_stack_c to default values (same as an
   * empty stack created with the constructor). If there are no items in this
   * QUANTAmisc_stack_c, an exception is issued.
   *
   * @exception QUANTAmisc_emptyStackException_c if there are no nodes in this List
   **/
  void clear(void); //throw(QUANTAmisc_emptyStackException_c);


protected:


private:
  int _size; // keeps track of the number of the items in the stack
  QUANTAmisc_stackNode_c<T> *_top_node; // node currently on top
  QUANTAmisc_stackNode_c<T> *_temp_node; // used to create new item in the sequence
  T _temp_data; // used to return node data
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_stack_c<T>::QUANTAmisc_stack_c(void)
{
  // set the node count to zero
  _size = 0;

  // create a new node with data, next, and prev all NULL
  _top_node = new QUANTAmisc_stackNode_c<T>();
  _temp_node = NULL;
}


template <class T>
QUANTAmisc_stack_c<T>::~QUANTAmisc_stack_c(void)
{
  try {
    clear();
  } catch(QUANTAmisc_exception_c ex) {}
  delete _top_node;
  delete _temp_node;
}


template <class T>
T &QUANTAmisc_stack_c<T>::pop(void) throw(QUANTAmisc_emptyStackException_c)
{
  // if empty, throw exception
  if(_size == 0 || _top_node == NULL)
    throw QUANTAmisc_emptyStackException_c("QUANTAmisc_stack_c::pop() - stack is empty.");

  _temp_node = _top_node;
  _temp_data = _temp_node->_data; // save the top item
  _top_node = _top_node->_next; // re-arrange the linked list

  delete _temp_node;
  _temp_node = NULL;
  _size--; // decrement size

  return _temp_data; // return the stacks top item
}


template <class T>
inline T &QUANTAmisc_stack_c<T>::peek(void) throw(QUANTAmisc_emptyStackException_c)
{
  // if the stack is empty, throw exception
  if(_size == 0 || _top_node == NULL)
    throw QUANTAmisc_emptyStackException_c("QUANTAmisc_stack_c::pop() - stack is empty.");

  return _top_node->_data; // return the top item in the stack
}


template <class T>
void QUANTAmisc_stack_c<T>::push(const T &data)
{
  if(_top_node == NULL)
    _top_node = new QUANTAmisc_stackNode_c<T>();

  // create a new linked list node and put it on the stack
  _temp_node = new QUANTAmisc_stackNode_c<T>(data, _top_node);
  _top_node = _temp_node;
  _size++; // increment the stacks size counter
}


template <class T>
inline bool QUANTAmisc_stack_c<T>::isEmpty(void)
{
  // if the stack is empty, return true, else false
  if(_size == 0 || _top_node == NULL)
    return true;

  return false;
}


template <class T>
inline int QUANTAmisc_stack_c<T>::size(void)
{
  // return current size
  return _size;
}


template <class T>
void QUANTAmisc_stack_c<T>::clear(void) //throw(QUANTAmisc_emptyStackException_c)
{
  //  if(_size == 0 || _top_node == NULL)
  //     throw EmptyStackException_c("QUANTAmisc_stack_c::pop() - stack is empty.");

  while(_top_node->_next != NULL)
  {
    _temp_node = _top_node; // assign node to delete
    _top_node = _top_node->_next;
    delete _temp_node;
    _size--;
  }
  delete _top_node;

  _top_node = NULL;
  _temp_node = NULL;
}


#endif
