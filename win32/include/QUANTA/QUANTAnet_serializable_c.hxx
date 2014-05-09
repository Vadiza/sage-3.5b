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

#ifndef _QUANTA_NET_SERIALIZABLE_H
#define _QUANTA_NET_SERIALIZABLE_H

#include <QUANTA/QUANTAnet_datapack_c.hxx>


//
// the data size MUST be set in order to use this class
//
class QUANTAnet_serializable_c : public QUANTAnet_datapack_c
{
public:
  QUANTAnet_serializable_c(void);
  QUANTAnet_serializable_c(unsigned long data_size);
  virtual ~QUANTAnet_serializable_c(void);


  /**
   * get the data size for this object
   * @return data size
   **/
  unsigned long getDataSize(void);


  /**
   * set the data size for this object
   * @param data size
   **/
  void setDataSize(unsigned long data_size);


  /**
   * read an object from a stream
   * @param buf object stream to read
   * @return status
   **/
  virtual void readObjectStream(char *buf) = 0;


  /**
   * write an object to a stream
   * @param buf object stream to write
   * @return status
   **/
  virtual void writeObjectStream(char *buf) = 0;


private:
  unsigned long _data_size;

};

#endif
