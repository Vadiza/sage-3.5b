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

#ifndef _QUANTA_MISC_EXCEPTION_H
#define _QUANTA_MISC_EXCEPTION_H


class QUANTAmisc_exception_c
{
public:
  //    QUANTAmisc_exception_c(void) { _str = "No exception message registered."; }
  QUANTAmisc_exception_c(char *str) { _str = str; }
  ~QUANTAmisc_exception_c(void) { /*delete [] _str;*/ }

  char *getMessage(void) const { return _str; }

private:
  char *_str;
};


class QUANTAmisc_noCurrentException_c : public QUANTAmisc_exception_c
{
public:
  //    QUANTAmisc_noCurrentException(void) : Exception() {}
  QUANTAmisc_noCurrentException_c(char *msg) : QUANTAmisc_exception_c(msg) {}
  ~QUANTAmisc_noCurrentException_c(void) {}

};


class QUANTAmisc_emptyStackException_c : public QUANTAmisc_exception_c
{
public:
  QUANTAmisc_emptyStackException_c(char *msg) : QUANTAmisc_exception_c(msg) {}
  ~QUANTAmisc_emptyStackException_c(void) {}

};


class QUANTAmisc_emptyQueueException_c : public QUANTAmisc_exception_c
{
public:
  QUANTAmisc_emptyQueueException_c(char *msg) : QUANTAmisc_exception_c(msg) {}
  ~QUANTAmisc_emptyQueueException_c(void) {}

};


class QUANTAmisc_emptyTreeException_c : public QUANTAmisc_exception_c
{
public:
  QUANTAmisc_emptyTreeException_c(char *msg) : QUANTAmisc_exception_c(msg) {}
  ~QUANTAmisc_emptyTreeException_c(void) {}

};


class QUANTAmisc_outOfBoundsException_c : public QUANTAmisc_exception_c
{
public:
  QUANTAmisc_outOfBoundsException_c(char *msg) : QUANTAmisc_exception_c(msg) {}
  ~QUANTAmisc_outOfBoundsException_c(void) {}

};


#endif
