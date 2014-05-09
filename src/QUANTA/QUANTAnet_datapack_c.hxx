/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the distribution.
 *  * Neither the name of the University of Illinois at Chicago nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#ifndef _QUANTA_DATAPACK_C_H
#define _QUANTA_DATAPACK_C_H

#define FLOAT_SIZE 4
#define DOUBLE_SIZE 8
#define INT_SIZE 4
#if (_MIPS_SZLONG==64)
#define LONG_SIZE 8
#define UNSIGNED_LONG_SIZE 8
#else
#define LONG_SIZE 4
#define UNSIGNED_LONG_SIZE 4
#endif
#define INT32_SIZE 4
#define INT64_SIZE 8
#define CHAR_SIZE 1

typedef int int32;

#ifdef WIN32
typedef __int64 int64;
#elif (_MIPS_SZLONG==64)
typedef long int64;
#else
typedef long long int64;
#endif

#ifdef _WIN32_WCE
#ifndef __HAVE_WINSOCK2_H
#include <winsock2.h>
#define __HAVE_WINSOCK2_H
#endif /* __HAVE_WINSOCK2_H */
#endif

/**
   Data packing class. It is basically a glorified memcpy().  The idea is
   that you create an object to help you pack data for transmission over
   networks.

   Sending and packing data: First you create a QUANTAnet_datapack_c
   object. Then using the InitPack() method, assign to it a pre-allocated
   memory buffer (SEE BELOW FOR IMPORTANT NOTES.)
   Then using the various QUANTAnet_datapack_c::pack*()
   member functions, you can pack integers, chars, floats, and doubles
   into the buffer. The buffer is now ready for big-endian to
   little-endian transmission. (And vice-versa).

   Receiving and unpacking data: Similarly if you receive a buffer of
   data from the network, you assign this buffer to a
   QUANTAnet_datapack_c object using the InitUnpack() method.  Finally,
   we unpack its constituent components using the
   QUANTAnet_datapack_c::unpack*() member functions.

   IMPORTANT NOTES:

   It is important to compute the length of the buffer using the various
   QUANTAnet_datapack_c::sizeof_*() methods where possible as additional
   buffer space is needed to encode platform-specific information. If in
   doubt always allocate 1 more byte than necessary. The sizeof_() calls
   will make sure that extra byte is included.

   To make your application as portable as possible, please take a look at
   packInt32 and packInt64 and counter functions that unpacks the data.
   These functions should be more portable than packInt and packLong functions
   since there is not specification about the size of int and lont int types
   in C or C++ language reference manual.

   For example, on SGI, int is going to be treated as 32bits no matter what
   kind of binary format you are using. However, long int is going to be treated
   as 32bits if you use 32 or n32 for your binary format, whereas it would take
   64bits if you use 64 as your binary format. On Win32 and linux running on
   Intel processors, both int and long takes 32bit space.

   Finally remember the order in which you packed your data. You need to
   use the same order to unpack them correctly.

   @version: 12/1/1999

*/

class QUANTAnet_datapack_c {


public:
  QUANTAnet_datapack_c() {
    running = 0; size = 0;
    start = 0;
  }


  ///Datapack class return values
  //@{
  /// Operation went ok
  static const int OK/* = 0*/;
  /// Operation failed
  static const int FAILED/* = -1*/;
  //@}

  /**
     Before we do any actual packing, we first call this method to attach the datapack object to some buffer

     @param buffer
     This buffer provided by the API user is where the packed data will be stored

     @param buffersize
     The size in bytes of the buffer above
  */
  void initPack(char *buffer, int buffersize);

  /**
     Before we do any actual unpacking, we first call this method to attach the datapack object to some buffer

     @param buffer
     This buffer where the packed data is stored

     @param buffersize
     The size in bytes of the buffer above
  */
  void initUnpack(char *buffer, int buffersize);

  /**
     Given a datapack class, this method gives us a pointer to the buffer where the packed data is stored

     @return
     A pointer to the beginning of the packed data stream


  */
  char *getBuffer() { return (char *)start;}

  /**
     Gives us the size of the buffer attached to this datapack object

     @return
     Size in bytes of the attached buffer

  */
  int getBufferMaxSize() { return size;}

  /**
     Gives us the size in bytes of available space left in the attached buffer

     @return
     Size in bytes of the remaining space available in the attached buffer
  */
  int getBufferFilledSize() {return running - start;}

  /**
     Insert a variable of type float into the buffer

     @return
     OK or FAILED

     @param val
     The float variable to be packed
  */
  int packFloat(float val);

  /**
     Insert a variable of type int into the buffer

     @return
     OK or FAILED

     @param val
     The int variable to be packed
  */
  int packInt(int val) ;

  /**
     Insert a variable of type 32-bit integer into the buffer

     @return
     OK or FAILED


     @param val
     The 32bit integer variable to be packed
  */
  int packInt32(int32 val);

  /**
     Insert a variable of type long into the buffer. Note that long takes 8
     bytes when it is compiled with 64-bit compiler on SGI.

     @return
     OK or FAILED

     @param val
     The long variable to be packed
  */
  int packLong(long val) ;

  /**
     Insert a variable of type 64-bit integer into the buffer

     @return
     OK or FAILED

     @param val
     The 64-bit integer variable to be packed
  */
  int packInt64(int64 val);

  /**
     Insert a variable of type double into the buffer

     @return
     OK or FAILED

     @param val
     The double variable to be packed
  */
  int packDouble(double val);


#ifdef _WIN32_WCE
  /**
     Insert a variable of type char into the buffer

     @return
     OK or FAILED

     @param val
     The Unicode char variable to be packed
  */
  int packChar(TCHAR val) ;
#endif

  /**
     Insert a variable of type char into the buffer

     @return
     OK or FAILED

     @param val
     The char variable to be packed
  */
  int packChar(char val) ;

  /** Pack a null-terminated character string.
      This convenience functions packs a null-terminated character
      string into the buffer. The length of the string in bytes is
      internally encoded. This reduces network traffic as only the
      required number of bytes are sent across the network.

      @param val null-terminated character string to pack
      @return OK or FAILED
  */
  int packString(char* val);

#ifdef _WIN32_WCE
  /**
     Pack raw characters into the buffer

     @return
     OK or FAILED

     @param val
     The Unicode char array to be packed
     @param sz
     Num chars to pack
  */
  int pack(TCHAR* val, int sz) ;
#endif

  /**
     Pack raw characters into the buffer

     @return
     OK or FAILED

     @param val
     The char array to be packed
     @param sz
     Num chars to pack
  */
  int pack(char* val, int sz) ;

  /**
     Pack floats of a float array into the buffer

     @return
     OK or FAILED

     @param val
     The float array to be packed
     @param sz
     Num floats in the array
  */
  int packFloatArray(float* val, int sz) ;

  /**
     Pack doubles of a double array into the buffer

     @return
     OK or FAILED

     @param val
     The double array to be packed
     @param sz
     Num doubles in the array
  */
  int packDoubleArray(double* val, int sz) ;

  /**
     Pack an array with int type values into the buffer

     @return
     OK or FAILED

     @param val
     The int array to be packed
     @param sz
     Num int values in the array
  */
  int packIntArray(int* val, int sz) ;

  /**
     Pack an array with int32 type values into the buffer

     @return
     OK or FAILED

     @param val
     The int32 array to be packed
     @param sz
     Num int32 values in the array
  */
  int packInt32Array(int32* val, int sz) ;

  /**
     Pack an array with int64 type values into the buffer

     @return
     OK or FAILED

     @param val
     The int64 array to be packed
     @param sz
     Num int64 values in the array
  */
  int packInt64Array(int64* val, int sz) ;

  /**
     Pack an array with long type values into the buffer

     @return
     OK or FAILED

     @param val
     The long array to be packed
     @param sz
     Num long values in the array
  */
  int packLongArray(long* val, int sz) ;

  /**
     Insert a variable of type unsigned long into the buffer. Note that long    takes 8 bytes when it is compiled with 64-bit compiler on SGI.

     @return
     OK or FAILED

     @param val
     The unsigned long variable to be packed
  */

  int packUnsignedLong(unsigned long val);

  /**
     Extract a variable of type float from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided float variable where we store the extracted value
  */
  int unpackFloat(float *Answer) ;

  /**
     Extract a variable of type int from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided int variable where we store the extracted value
  */
  int unpackInt(int *Answer) ;

  /**
     Extract a variable of type 32bit integer from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided 32bit integer variable where we store the extracted value
  */
  int unpackInt32(int32 *Answer) ;

  /**
     Extract a variable of type long from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided long variable where we store the extracted value
  */
  int unpackLong(long *Answer) ;

  /**
     Extract a variable of type unsigned long from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided unsigned long variable where we store the extracted value
  */
  int unpackUnsignedLong(unsigned long *Answer) ;


  /**
     Extract a variable of type 64-bit integer from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided 64-bit integer variable where we store the extracted value
  */
  int unpackInt64(int64 *Answer) ;

  /**
     Extract a variable of type double from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided double variable where we store the extracted value
  */
  int unpackDouble(double *Answer)  ;


#ifdef _WIN32_WCE
  /**
     Extract a variable of type char from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided Unicode char variable where we store the extracted value
  */
  int unpackChar(TCHAR *Answer) ;
#endif

  /**
     Extract a variable of type char from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided char variable where we store the extracted value
  */
  int unpackChar(char *Answer) ;

  /** Pack a null-terminated character string.
      This convenience functions packs a null-terminated character
      string into the buffer. The length of the string in bytes is
      internally encoded when extracting the string. If sz indicates
      that val does not have enough space to unpack the string, then
      the unpack function will fail. The unpacked value does include
      the terminating null character '\\0'.

      @param val character buffer to stored unpacked string.
      @param sz size in bytes of the val buffer.
      @return OK or FAILED
  */
  int unpackString(char* val, const int& sz);

#ifdef _WIN32_WCE
  /**
     Extract the packed chars from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided Unicode char array where we store the extracted value
  */
  int unpack(TCHAR *Answer, int sz) ;
#endif

  /**
     Extract the packed chars from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided char array where we store the extracted value
     @param sz
     Size in bytes of the provided char array
  */
  int unpack(char *Answer, int sz) ;

  /**
     Extract the packed float array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided float array where the extracted floats are stored

     @param sz
     The size of the array in which the extracted floats are stored
  */
  int unpackFloatArray(float* Answer, int sz);

  /**
     Extract the packed double array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided double array where the extracted doubles are stored

     @param sz
     The size of the array in which the extracted doubles are stored
  */
  int unpackDoubleArray(double* Answer, int sz);

  /**
     Extract the packed int array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided int array where the extracted int values are stored

     @param sz
     The size of the array in which the extracted int values are stored
  */
  int unpackIntArray(int* Answer, int sz);

  /**
     Extract the packed int32 array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided array where the extracted int32 values are stored

     @param sz
     The size of the array in which the extracted int32 values are stored
  */
  int unpackInt32Array(int32* Answer, int sz);

  /**
     Extract the packed int64 array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided array where the extracted int64 values are stored

     @param sz
     The size of the array in which the extracted int64 values are stored
  */
  int unpackInt64Array(int64* Answer, int sz);

  /**
     Extract the packed long array from the buffer

     @return
     OK or FAILED

     @param Answer
     A user provided array where the extracted long values are stored

     @param sz
     The size of the array in which the extracted long values are stored
  */
  int unpackLongArray(long* Answer, int sz);

  /**
     This function tells us if there is enough incoming_size bytes in the buffer to perform the operation.

     @return OK or FAILED

     @param incoming_size
     The size in bytes of to be tested.
  */
  int checkspace(unsigned int incoming_size);

  /**
     Gives us a cross-platform safe float size

     @return
     The number of bytes in an int (+ 1 byte for machine code storage)
  */
  static int sizeof_float(unsigned int cnt=1){return FLOAT_SIZE * cnt;};
  /**
     Gives us a cross-platform safe int size

     @return
     The number of bytes in an int (+ 1 byte for machine code storage)
  */
  static int sizeof_int(unsigned int cnt=1){return INT_SIZE * cnt;};

  /**
     Gives us a cross-platform safe 64-bit int size

     @return
     The number of bytes in an 64-bit int
  */
  static int sizeof_int64(unsigned int cnt=1){return INT64_SIZE * cnt;};

  /**
     Gives us a cross-platform safe 32-bit int size

     @return
     The number of bytes in an 32-bit int
  */
  static int sizeof_int32(unsigned int cnt=1){return INT32_SIZE * cnt;};

  /**
     Gives us a cross-platform safe long size

     @return
     The number of bytes in a long (+ 1 byte for machine code storage)
  */
  static int sizeof_long(unsigned int cnt=1){return LONG_SIZE * cnt;};

  /**
     Gives us a cross-platform safe unsigned long size

     @return
     The number of bytes in unsigned long (+ 1 byte for machine code storage)
  */
  static int sizeof_unsignedLong(unsigned int cnt=1){return UNSIGNED_LONG_SIZE * cnt;};

  /**
     Gives us a cross-platform safe char size

     @return
     The number of bytes in a char 1 byte for machine code storage)
  */
  static int sizeof_char(unsigned int cnt=1){return CHAR_SIZE * cnt;};

  /**
     Gives us a cross-platform safe double size

     @return
     The number of bytes in a double (+ 1 byte for machine code storage)
  */
  static int sizeof_double(unsigned int cnt=1){return DOUBLE_SIZE * cnt;};

  /** Gives us a cross-platform safe string size encoded with its length.
      @return The number of bytes used to encode a string.
  */
  static int sizeof_string(unsigned int cnt=1)
  { return sizeof_int() + sizeof_char(cnt); }


private:
  unsigned long size;
  unsigned char *running;
  unsigned char *start;

};

#endif
