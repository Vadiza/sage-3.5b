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

#ifndef _QUANTA_MISC_VECTOR_H
#define _QUANTA_MISC_VECTOR_H

#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif
#include <stdlib.h>

#include <QUANTA/QUANTAmisc_exception_c.hxx>
#include <QUANTA/QUANTAglobals.hxx>


// an array template to make it easier to create and maintain
// arrays of connected nodes/clients; meant for internal library
// use by quanta classes

// efficency may become an issue for larger arrays if using remove(T)

template <class T>
class QUANTAmisc_vector_c
{
public:
  QUANTAmisc_vector_c(unsigned int capacity = 10, unsigned int capacity_increment = 10);
  ~QUANTAmisc_vector_c(void);

  void addElement(const T &obj); // add an element
  void insertElementAt(const T &obj, unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c);
  void removeElement(const T &obj) throw(QUANTAmisc_noCurrentException_c); // scan and remove an element
  void removeElementAt(unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c); // remove a known element
  void setElementAt(const T &obj, unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c);
  void clear(void); // remove all elements
  void ensureCapacity(unsigned int capacity_increment = 10);
  void trimToSize(void);

  T &elementAt(unsigned int index) const throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c);
  inline unsigned int capacity(void) const { return _capacity; } // total number of slots currently allocated
  bool contains(const T &element) const;
  T &firstElement(void) const throw(QUANTAmisc_noCurrentException_c);
  T &lastElement(void) const throw(QUANTAmisc_noCurrentException_c);
  inline unsigned int size(void) const { return _size; }; // return current number of elements
  bool isEmpty(void) const;
  int indexOf(const T &obj) const;
  int lastIndexOf(const T &obj) const;

  T& operator[](unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c); // return element at known position
  //    const T& operator[](unsigned int index ) const throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c);


private:
  T *_data; // the array
  unsigned int _size; // current number of elements
  unsigned int _capacity;
  unsigned int _capacity_increment;
};


template <class T>
QUANTAmisc_vector_c<T>::QUANTAmisc_vector_c(unsigned int capacity = 10, unsigned int capacity_increment = 10)
{
  _size = 0;
  _capacity = capacity;
  _capacity_increment = capacity_increment;
  _data = (T*)calloc(_capacity, sizeof(T));
}


template <class T>
QUANTAmisc_vector_c<T>::~QUANTAmisc_vector_c(void)
{
  free(_data);
}


template <class T>
void QUANTAmisc_vector_c<T>::ensureCapacity(unsigned int capacity_increment = 10)
{
  _capacity += capacity_increment;
  _data = (T*)realloc(_data, sizeof(T) * _capacity);
}


template <class T>
void QUANTAmisc_vector_c<T>::addElement(const T &obj)
{
  int index = _size; // save the current size as the array index

  if(_size == 0 || _data == NULL)
  {
    _size++;
    _data = (T*)calloc(_capacity, sizeof(T));
  }
  else
  {
    _size++;
    if(_size >= _capacity)
    {
      _capacity += _capacity_increment;
      _data = (T*)realloc(_data, sizeof(T) * _capacity);
    }
  }

  _data[index] = obj;
}


template <class T>
void QUANTAmisc_vector_c<T>::insertElementAt(const T &obj, unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c)
{
  if(index == _size)
    addElement(obj);
  else
  {
    if(_size == 0 || _data == NULL)
      throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::remove(int) - invalid current node.");

    if(index >= _size)
      throw QUANTAmisc_outOfBoundsException_c("QUANTAmisc_vector_c::remove(int) - index out of array bounds.");

    // increase size
    _size++;
    // create memory space for the new element
    if(_size >= _capacity)
    {
      _capacity += _capacity_increment;
      _data = (T*)realloc(_data, sizeof(T) * _capacity);
    }

    // starting at the end, move every element over one space until index is reached
    for(unsigned int j = _size; j > index + 1; j--)
      _data[j] = _data[j - 1];

    _data[index] = obj;
  }
}


template <class T>
void QUANTAmisc_vector_c<T>::removeElementAt(unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::remove(int) - invalid current node.");

  if(index >= _size)
    throw QUANTAmisc_outOfBoundsException_c("QUANTAmisc_vector_c::remove(int) - index out of array bounds.");

  _size--;
  for(unsigned int j = index; j < _size; j++)
    _data[j] = _data[j+1];
}


template <class T>
void QUANTAmisc_vector_c<T>::setElementAt(const T &obj, unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::setElementAt() - invalid current node.");

  if(index >= _size)
    throw QUANTAmisc_outOfBoundsException_c("QUANTAmisc_vector_c::setElementAt() - index out of array bounds.");

  _data[index] = obj;
}


template <class T>
T &QUANTAmisc_vector_c<T>::firstElement(void) const throw(QUANTAmisc_noCurrentException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::firstElement() - invalid current node.");

  return _data[0];
}


template <class T>
T &QUANTAmisc_vector_c<T>::lastElement(void) const throw(QUANTAmisc_noCurrentException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::lastElement() - invalid current node.");

  return _data[_size - 1];
}


template <class T>
T &QUANTAmisc_vector_c<T>::elementAt(unsigned int index) const throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::ElementAt() - invalid current node.");

  if(index >= _size)
    throw QUANTAmisc_outOfBoundsException_c("QUANTAmisc_vector_c::ElementAt() - index out of array bounds.");

  return _data[index];
}


// Note that all comparisons use the operator ==.
template <class T>
void QUANTAmisc_vector_c<T>::removeElement(const T &obj) throw(QUANTAmisc_noCurrentException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::remove(T) - invalid current node.");

  for(unsigned int i = 0; i < _size; i++)
  {
    if(_data[i] == obj)
    {
      _size--;
      for(int j = i; j < _size; j++)
        _data[j] = _data[j+1];
      return;
    }
  }
}


template <class T>
T& QUANTAmisc_vector_c<T>::operator[](unsigned int index) throw(QUANTAmisc_noCurrentException_c, QUANTAmisc_outOfBoundsException_c)
{
  if(_size == 0 || _data == NULL)
    throw QUANTAmisc_noCurrentException_c("QUANTAmisc_vector_c::operator[]() - invalid current node.");

  if(index >= _size)
    throw QUANTAmisc_outOfBoundsException_c("QUANTAmisc_vector_c::operator[]() - index out of array bounds.");

  return _data[index];
}


template <class T>
void QUANTAmisc_vector_c<T>::clear(void)
{
  free(_data);
  _data = NULL;
  _size = 0;
}


template <class T>
inline bool QUANTAmisc_vector_c<T>::isEmpty(void) const
{
  if(_size == 0 || _data == NULL)
    return true;

  return false;
}


// Note that all comparisons use the operator ==.
template <class T>
bool QUANTAmisc_vector_c<T>::contains(const T &element) const
{
  for(unsigned int i = 0; i < _size; i++)
  {
    if(_data[i] == obj)
      return true;
  }

  return false;
}


// Note that all comparisons use the operator ==.
template <class T>
int QUANTAmisc_vector_c<T>::indexOf(const T &obj) const
{
  for(unsigned int i = 0; i < _size; i++)
  {
    if(_data[i] == obj)
      return i;
  }

  return -1;
}


// Note that all comparisons use the operator ==.
template <class T>
int QUANTAmisc_vector_c<T>::lastIndexOf(const T &obj) const
{
  for(unsigned int i = size; i >= 0; i--)
  {
    if(_data[i] == obj)
      return i;
  }

  return -1;
}


template <class T>
void QUANTAmisc_vector_c<T>::trimToSize(void)
{
  if(_capacity != _size)
  {
    _capacity = _size;
    _data = (T*)realloc(_data, sizeof(T) * _capacity);
  }
}


#endif
