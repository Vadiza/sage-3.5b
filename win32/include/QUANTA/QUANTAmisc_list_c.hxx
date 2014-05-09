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

#ifndef _QUANTA_MISC_LIST_H
#define _QUANTA_MISC_LIST_H

#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif
#include <stdlib.h>

#include <QUANTA/QUANTAmisc_exception_c.hxx>
#include <QUANTA/QUANTAglobals.hxx>

/**
 * Double ended linked list class.
 **/

template <class T>
class QUANTAmisc_list_c;

/**
 * The <CODE>QUANTAmisc_listNode_c</CODE> class is used as a storage object for data.
 * It further keeps track of the previous and next QUANTAmisc_listNode_cs in the sequence
 * with the use of <CODE>prev</CODE> and <CODE>next</CODE> QUANTAmisc_listNode_c data
 * members. For internal use only by QUANTAmisc_list_c class.
 **/
template <class T>
class QUANTAmisc_listNode_c
{
  friend class QUANTAmisc_list_c<T>;

public:
  /**
   * all nodes are deleted by the QUANTAmisc_list_c class
   **/
  ~QUANTAmisc_listNode_c(void);


private:
  /**
   * Creates a new, completely empty QUANTAmisc_listNode_c.  The data, prev, and next
   * references are all set to <CODE>NULL</CODE>.
   **/
  QUANTAmisc_listNode_c(void);


  /**
   * Creates a new QUANTAmisc_listNode_c which contains the specified data.
   * The prev and next references are set to <CODE>NULL</CODE>.
   *
   * @param data the data being stored in this QUANTAmisc_listNode_c
   **/
  QUANTAmisc_listNode_c(const T &data);


  /**
   * Creates a new QUANTAmisc_listNode_c which contains the specified data with
   * specified prev and next references.
   *
   * @param data the data being stored in this QUANTAmisc_listNode_c
   * @param prev the node to be previous to this QUANTAmisc_listNode_c
   * @param next the node to follow this QUANTAmisc_listNode_c
   **/
  QUANTAmisc_listNode_c(const T &data, QUANTAmisc_listNode_c<T> *prev, QUANTAmisc_listNode_c<T> *next);


  /**
   * The data this node is keeping track of.
   **/
  T _data;

  /**
   * The node preceeding this QUANTAmisc_listNode_c.  If there is no such node,
   * <CODE>prev</CODE> should have the value <CODE>NULL</CODE>.
   * Otherwise, it should be the case that <CODE>prev->next == this</CODE>
   **/
  QUANTAmisc_listNode_c<T> *_prev;

  /**
   * The node following this QUANTAmisc_listNode_c.  If there is no such node,
   * <CODE>next</CODE> should have the value <CODE>NULL</CODE>.
   * Otherwise it should be the case that <CODE>next->prev == this</CODE>
   **/
  QUANTAmisc_listNode_c<T> *_next;
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_listNode_c<T>::QUANTAmisc_listNode_c(void)
{
  _prev = NULL;
  _next = NULL;
}


template <class T>
QUANTAmisc_listNode_c<T>::QUANTAmisc_listNode_c(const T &data)
  : _data(data)
{
  _prev = NULL;
  _next = NULL;
}


template <class T>
QUANTAmisc_listNode_c<T>::QUANTAmisc_listNode_c(const T &data, QUANTAmisc_listNode_c<T> *prev, QUANTAmisc_listNode_c<T> *next)
  : _data(data), _prev(prev), _next(next)
{
}


template <class T>
QUANTAmisc_listNode_c<T>::~QUANTAmisc_listNode_c(void)
{
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <class T>
class QUANTAmisc_list_c
{
public:
  /**
   * Creates a new, initially empty QUANTAmisc_list_c.  The current node is undefined.
   **/
  QUANTAmisc_list_c(void);


  /**
   * Destroy a QUANTAmisc_list_c.
   **/
  ~QUANTAmisc_list_c(void);


  /**
   * Returns the number of items currently in this QUANTAmisc_list_c.
   *
   * @return the number of items currently in this QUANTAmisc_list_c
   **/
  inline int size(void);


  /*******
   * Checks if the list is empty.
   *
   * @return true if the list is empty; otherwise, returns false.
   *******/
  inline bool isEmpty(void);


  /**
   * Returns true if there is a current item; otherwise, returns false.
   * @return Is there a current item?
   **/
  inline bool isCurrent(void);


  /**
   * Moves current to the next item in this QUANTAmisc_list_c.
   *
   * @exception QUANTAmisc_noCurrentException_c if there are no items in this QUANTAmisc_list_c
   * or if you try to advance beyond the end of the sequence
   **/
  inline T &next(void) throw(QUANTAmisc_noCurrentException_c);


  /**
   * Moves current to the previous item in this QUANTAmisc_list_c.
   *
   * @exception QUANTAmisc_noCurrentException_c if there are no items in this QUANTAmisc_list_c
   * or if you try to back up beyond the beginning of the sequence
   **/
  inline T &previous(void) throw(QUANTAmisc_noCurrentException_c);


  /**
   * Makes the first item in the sequence be the current item.
   **/
  inline void first(void);


  /**
   * Makes the last item in the sequence be the current item.
   **/
  inline void last(void);


  /**
   * Returns but does not remove the current item in this Sequence.
   * An exception is issued if the Sequence is currently empty.
   *
   * @return the current item in this Sequence
   * @exception QUANTAmisc_noCurrentException_c if there are no items in this QUANTAmisc_list_c
   **/
  inline T &getCurrent(void) throw(QUANTAmisc_noCurrentException_c);


  /**
   * Inserts the specified data after the current element.  Simply adds the
   * item if the QUANTAmisc_list_c is empty.  The newly inserted item is considered
   * to be the current item after the addition.
   *
   * @param data the item to insert into this QUANTAmisc_list_c
   **/
  void addAfter(const T &data);


  /**
   * Inserts the specified element prior to the current element.  Simply adds
   * the item if the QUANTAmisc_list_c is empty.  The newly inserted item is
   * considered to be the current item after the addition.
   *
   * @param data the item to inset into this QUANTAmisc_list_c
   **/
  void addBefore(const T &data);


  /**
   * Removes and returns the current item in this QUANTAmisc_list_c.  The current
   * item in this QUANTAmisc_list_c is then updated to be the item following current.
   * If the last item in this QUANTAmisc_list_c is removed, current becomes
   * undefined (i.e. if current is the tail node of the QUANTAmisc_list_c, after
   * calling removeCurrent, the current pointer will be undefined).  If there
   * are no items in this QUANTAmisc_list_c, an exception is issued.
   *
   * @return the former current item in the sequence
   * @exception QUANTAmisc_noCurrentException_c if there are no nodes in this QUANTAmisc_list_c
   **/
  T &removeCurrent(void) throw(QUANTAmisc_noCurrentException_c);


  /**
   * Removes all nodes and resets this QUANTAmisc_list_c to default values (same as an
   * empty list created with the constructor). If there are no items in this
   * QUANTAmisc_list_c, an exception is issued.
   *
   * @exception QUANTAmisc_noCurrentException_c if there are no nodes in this QUANTAmisc_list_c
   **/
  void clear(void); //throw(QUANTAmisc_noCurrentException_c);


  /**
   * Tests whether the specified item is in this QUANTAmisc_list_c by advancing
   * forward from the current item through the sequence, wrapping around to
   * the beginning of the sequence and continuing the search from the
   * beginning, if necessary.  If the item is in this QUANTAmisc_list_c, current is
   * updated to refer to this item, and true is returned.  Otherwise, false
   * is returned and no changes are made to the current reference.  False
   * is also returned if there are no items in this QUANTAmisc_list_c.  Note that
   * all comparisons use the operator ==.
   *
   * @param item the item to find
   * @return true if the item is found, false otherwise
   **/
  bool contains(const T &item);


private:
  // number of nodes in the sequence
  int _size;

  QUANTAmisc_listNode_c<T> *_head_node;
  QUANTAmisc_listNode_c<T> *_tail_node;
  QUANTAmisc_listNode_c<T> *_curr_node;
  QUANTAmisc_listNode_c<T> *_temp_node; // used to create new item in the sequence
  T _temp_data; // used to return node data
};


////////////////////////////////////////////////////////////////////////////////
template <class T>
QUANTAmisc_list_c<T>::QUANTAmisc_list_c(void)
{
  // set the node count to zero
  _size = 0;

  // create a new node with data, next, and prev all NULL
  _head_node = new QUANTAmisc_listNode_c<T>();

  // set the current and tail nodes to the head node,
  // which contains NULLs for data, next, and prev
  _tail_node = _head_node;
  _curr_node = _temp_node = NULL;
}


template <class T>
QUANTAmisc_list_c<T>::~QUANTAmisc_list_c(void)
{
  try {
    clear();
  } catch(QUANTAmisc_exception_c ex) {}
  // no need to delete _tail_node.
  // _tail_node points to _head_node after clear() is called
  delete _head_node;
  delete _curr_node;
  delete _temp_node;
}


template <class T>
inline int QUANTAmisc_list_c<T>::size(void)
{
  return _size;
}


template <class T>
inline bool QUANTAmisc_list_c<T>::isEmpty(void)
{
  if(_size == 0 || _curr_node == NULL)
    return true;

  return false;
}


template <class T>
inline bool QUANTAmisc_list_c<T>::isCurrent(void)
{
  //  if(_size == 0 || _curr_node == NULL)
  if(_size == 0 || _curr_node == NULL || _curr_node->_prev == NULL || _curr_node->_prev == _head_node)
    return false;

  return true;
}


template <class T>
inline T &QUANTAmisc_list_c<T>::next(void) throw(QUANTAmisc_noCurrentException_c)
{
  // if current node is NULL (list is empty) throw exception
  if(_curr_node == NULL || _curr_node->_next == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_list_c::next() - no current nodes.");

  // move the current node pointer to the next node
  _curr_node = _curr_node->_next;
  return _curr_node->_data;
}


template <class T>
inline T &QUANTAmisc_list_c<T>::previous(void) throw(QUANTAmisc_noCurrentException_c)
{
  // if current node is NULL (list is empty) throw exception
  if(_size == 0 || _curr_node == NULL || _curr_node->_prev == NULL || _curr_node->_prev == _head_node)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_list_c::previous() - no current nodes.");

  // move the current node pointer to the previous node
  _curr_node = _curr_node->_prev;
  return _curr_node->_data;
}


template <class T>
inline void QUANTAmisc_list_c<T>::first(void)
{
  // do nothing if the list is empty
  if(_size == 0 || _curr_node == NULL)
    return;

  // set the current node pointer to the first node in the list,
  // which is the head node's next
  _curr_node = _head_node->_next;
}


template <class T>
inline void QUANTAmisc_list_c<T>::last(void)
{
  // do nothing if the list is empty
  if(_size == 0 || _curr_node == NULL)
    return;

  // set the current node to the tail node
  _curr_node = _tail_node;
}


template <class T>
inline T &QUANTAmisc_list_c<T>::getCurrent(void) throw(QUANTAmisc_noCurrentException_c)
{
  // if the sequence contains no nodes or the current pointer
  // is located on the head node (indication that no node was
  // ever added, or all nodes were removed) throw an exception
  if(_size == 0 || _curr_node == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_list_c::getCurrent() - no current nodes.");

  // else return the current nodes data
  return _curr_node->_data;
}


template <class T>
void QUANTAmisc_list_c<T>::addAfter(const T &data)
{
  // if current node is NULL (list is empty) set current to head pointer
  if(_curr_node == NULL)
  {
    _head_node = new QUANTAmisc_listNode_c<T>();
    _curr_node = _head_node;
  }

  // create a new node to add to the sequence
  _temp_node = new QUANTAmisc_listNode_c<T>(data, _curr_node, _curr_node->_next);

  // if _curr_node.next is not NULL, set the next node's previous pointer to temp
  if(_curr_node->_next != NULL)
  {
    // set the next node's previous pointer to the temp node
    _curr_node->_next->_prev = _temp_node;
  }

  // set the current node's next pointer to the temp node
  _curr_node->_next = _temp_node;
  // advance the current pointer to the temp node
  _curr_node = _temp_node;

  // test the new current node to find out if it is the
  // last node in the sequence
  if(_curr_node->_next == NULL)
    _tail_node = _curr_node;

  // increment the size counter; added a node to the sequence
  _size++;
  // set temp to NULL
  _temp_node = NULL;
}


template <class T>
void QUANTAmisc_list_c<T>::addBefore(const T &data)
{
  // if current node is NULL (list is empty)
  // add this new node to the current pointer, which is the
  // head pointer if the list is empty
  //
  // this also assures the tail node gets set properly for adding a
  // node to and empty list
  //
  if(_curr_node == NULL)
  {
    addAfter(data);
    // skip out of this method, otherwise 2 nodes will be created
    // the first by addAfter() and then a second before that one in
    // this method's body
    return;
  }

  // create a new node
  _temp_node = new QUANTAmisc_listNode_c<T>(data, _curr_node->_prev, _curr_node);
  // if the previous node is not NULL set the previous node's
  // next pointer to the temp node
  if(_curr_node->_prev != NULL)
    _curr_node->_prev->_next = _temp_node;

  // set the current node's previous pointer to the temp node
  _curr_node->_prev = _temp_node;
  // set the current node to the temp node
  _curr_node = _temp_node;

  // increment the size counter; added a node to the sequence
  _size++;
  // set temp to NULL
  _temp_node = NULL;
}


template <class T>
T &QUANTAmisc_list_c<T>::removeCurrent(void) throw(QUANTAmisc_noCurrentException_c)
{
  // if the sequence is empty throw an exception
  if(_size == 0 || _curr_node == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_list_c::removeCurrent() - no current nodes.");

  // assign current node to a temp node so that this node can be deleted
  _temp_node = _curr_node;
  // this node's data to return
  _temp_data = _temp_node->_data;

  // assign the current node's previous node's next counter to the node
  // after the current node
  _curr_node->_prev->_next = _curr_node->_next;

  // if current's next node is NULL (the current node is the tail node),
  // assign the new current node to current's previous node, and re-assign
  // the tail pointer to the new current pointer
  if(_curr_node->_next == NULL)
  {
    _curr_node = _curr_node->_prev;
    _tail_node = _curr_node;
  }
  else
  {
    // current's next node is another valid node, so re-assign current's
    // next's node's previous pointer to the node before current
    // and re-assign the current pointer to current's next node
    _curr_node->_next->_prev = _curr_node->_prev;
    _curr_node = _curr_node->_next;
  }

  // decrement this sequence's node counter
  _size--;

  // trash the node that was dropped from the sequence
  delete _temp_node;
  _temp_node = NULL;

  // return the original current node's data
  return _temp_data;
}


template <class T>
void QUANTAmisc_list_c<T>::clear(void) //throw(QUANTAmisc_noCurrentException_c)
{
  //  if(_size == 0 || _curr_node == NULL)
  //    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_list_c::clear() - no current nodes.");

  _curr_node = _head_node->_next;
  while(_curr_node != NULL)
  {
    _temp_node = _curr_node; // assign node to delete
    _curr_node = _curr_node->_next;
    delete _temp_node;
    _size--;
  }
  delete _head_node;
  delete _curr_node;

  _tail_node = _head_node = NULL;
  _curr_node = _temp_node = NULL;
}


template <class T>
bool QUANTAmisc_list_c<T>::contains(const T &item)
{
  // if current node is null (list is empty) return false
  if(_size == 0 || _curr_node == NULL)
    return false;

  // use a temp to cycle thru the sequence so the current pointer
  // doesn't change if the lookup fails
  _temp_node = _curr_node;

  // cycle thru each node in the sequence starting with the current node
  while(_temp_node != NULL)
  {
    // return true if the data matches,
    // and set the current pointer to this node
    if(_temp_node->_data == item)
    {
      _curr_node = _temp_node;
      _temp_node = NULL;
      return true;
    }

    // if at the end of list, start over at the beginning
    if(_temp_node->_next == NULL)
    {
      // go to the fist node in the list
      _temp_node = _head_node->_next;
    }
    else
    {
      // go to the next node
      _temp_node = _temp_node->_next;
    }

    // if a complete cycle has been checked and the data
    // was not found, return false
    if(_temp_node == _curr_node)
      break;
  }

  // clean up
  _temp_node = NULL;
  return false;
}


#endif
