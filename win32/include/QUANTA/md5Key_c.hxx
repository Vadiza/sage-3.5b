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

#ifndef _MD5KEY_C
#define _MD5KEY_C

#include <QUANTA/md5.hxx>

#define MD5KEY_SIZE     16
#define MD5KEY_SIZE_2   33

/** MD5 encryption key. Give it a string and it will encrypt it into a 16 byte key.
 * Note: this class requires md5.h and md5.c which does the actual encryption work.
 */
class md5Key_c {
public:

  //  static const int MD5KEY_SIZE/* = 16*/;
  //  static const int MD5KEY_SIZE_2/* = 33*/; // 16 * 2 + 1 (extra byte for null character in string) [see getKeyAsStr()]

  md5Key_c();
  /// Assignment operator.
  int operator=(md5Key_c& copy);

  /// Hash a string of len with md5 encryption.
  int hash(char *str, int len);

  /// Conversion to integer (for use with hash tables)
  operator int();

  /// Determine if 2 md5 keys are the same.
  int operator==(md5Key_c &nkey);

  /// Assignment from string.
  int operator=(unsigned char* copy);

  /// Print the md5 key.
  void display();

  /// Retrieve MD5 key as a string that is printable.
  char *getKeyAsStr();

  /// MD5KEY_SIZE byte md5 encrypted key.
  unsigned char key[MD5KEY_SIZE];
  char keystr[MD5KEY_SIZE_2];

};

#endif
