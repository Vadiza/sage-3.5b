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

#include <stdio.h>
#include <string.h>
#include "QUANTAnet_datapack_c.hxx"

const int QUANTAnet_datapack_c::FAILED = -1;
const int QUANTAnet_datapack_c::OK = 0;

/* 1. QUANTA_LITTLE_ENDIAN_FORMAT must be defined for all
   machines that uses little endian byte ordering scheme.
   Most popular machines using little endian are Intel and
   Alpha processor.

   2. Note that using different byte order is not dependent on
   Operating system. It is closely tied to the processor type
   the system is based on. Thus, linux which runs on different
   processors may have different byte ordering scheme.

   3. With a simple test program, it looks like that at least
   Sparc machines from SUN, SGI, and Intel use the same
   format to store the floating point values except they must
   be stored using different byte ordering scheme. Probably
   they are all following the IEEE standard to store all
   floating point values.

   4. Thus, if you want to port these functions to some other
   platforms with different processors, you have to carefully
   check what format they are using to store floating point
   values and how each byte is stored in memory for the specific
   processors.
*/

typedef union {
  int value;
  unsigned char bytes[INT_SIZE];
} INT_VALUE;

typedef union {
  int32 value;
  unsigned char bytes[INT32_SIZE];
} INT32_VALUE;

typedef union {
  int64 value;
  unsigned char bytes[INT64_SIZE];
} INT64_VALUE;

typedef union {
  long value;
  unsigned char bytes[LONG_SIZE];
} LONG_VALUE;

typedef union {
  unsigned long value;
  unsigned char bytes[UNSIGNED_LONG_SIZE];
} UNSIGNED_LONG_VALUE;

typedef union {
  float value;
  unsigned char bytes[FLOAT_SIZE];
} FLOAT_VALUE;

typedef union {
  double value;
  unsigned char bytes[DOUBLE_SIZE];
} DOUBLE_VALUE;

void QUANTAnet_datapack_c::initPack(char *buffer, int buffersize)
{
  if (buffer == NULL)
    fprintf(stderr, "QUANTAnet_datapack_c::initPack WARNING: "
            "buffer is NULL\n");
  start = (unsigned char *) buffer;
  running = (unsigned char *) buffer;
  size = buffersize;
}

void QUANTAnet_datapack_c::initUnpack(char *buffer, int buffersize)
{
  if (buffer == NULL)
    fprintf(stderr, "QUANTAnet_datapack_c::initUnpack WARNING: "
            "buffer is NULL\n");
  start = (unsigned char *) buffer;
  running = (unsigned char *) buffer;
  size = buffersize;
}


/// Floats
int QUANTAnet_datapack_c::packFloat(float val)
{
  FLOAT_VALUE oldVal;

  if (checkspace(FLOAT_SIZE) != OK) return FAILED; // not enough buffer space

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  // reverse the order to make the format to BigEndian
  for (i = FLOAT_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, FLOAT_SIZE);
#endif // QUANTA_LITTLE_ENDIAN_FORMAT

  running += FLOAT_SIZE; // increment the pointer
  return OK;

}

/// Int
int QUANTAnet_datapack_c::packInt(int val)
{

  INT_VALUE oldVal;

  if (checkspace(INT_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = INT_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, INT_SIZE);
#endif

  running += INT_SIZE;
  return OK;
}

/// Int32
int QUANTAnet_datapack_c::packInt32(int32 val)
{

  INT32_VALUE oldVal;

  if (checkspace(INT32_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = INT32_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, INT32_SIZE);
#endif

  running += INT32_SIZE;
  return OK;
}

/// Long
int QUANTAnet_datapack_c::packLong(long val)
{

  LONG_VALUE oldVal;

  if (checkspace(LONG_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = LONG_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, LONG_SIZE);
#endif

  running += LONG_SIZE;
  return OK;
}

/// Unsigned long
int QUANTAnet_datapack_c::packUnsignedLong(unsigned long val)
{

  UNSIGNED_LONG_VALUE oldVal;

  if (checkspace(UNSIGNED_LONG_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = UNSIGNED_LONG_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, UNSIGNED_LONG_SIZE);
#endif

  running += UNSIGNED_LONG_SIZE;
  return OK;
}



/// Int64
int QUANTAnet_datapack_c::packInt64(int64 val)
{
  INT64_VALUE oldVal;

  if (checkspace(INT64_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = INT64_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, INT64_SIZE);
#endif

  running += INT64_SIZE;
  return OK;
}

/// Double
int QUANTAnet_datapack_c::packDouble(double val)
{

  DOUBLE_VALUE oldVal;

  if (checkspace(DOUBLE_SIZE) != OK) return FAILED;

  oldVal.value = val;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;

  for (i = DOUBLE_SIZE - 1, j = 0; i >= 0; i--, j++)
    running[j] = oldVal.bytes[i];
#else
  memcpy(running, oldVal.bytes, DOUBLE_SIZE);
#endif

  running += DOUBLE_SIZE;
  return OK;
}


/// Char
int QUANTAnet_datapack_c::packChar(char val)
{

  if (checkspace(CHAR_SIZE) != OK)
    return FAILED;

  *running++ = (unsigned char) val;
  return OK;
}

/// UNICODE Char
#ifdef _WIN32_WCE
int QUANTAnet_datapack_c::packChar(TCHAR val) // for unicode support
{
  if (checkspace(CHAR_SIZE) != OK)
    return FAILED;

  char newVal;

  WideCharToMultiByte(CP_ACP, 0, &val, 1, &newVal, 1, NULL, NULL);
  *running++ = (unsigned char) newVal;

  return OK;
}
#endif  // _WIN32_WCE

/*
  int QUANTAnet_datapack_c::packString(char* val)
  {
  if (val == NULL)
  return FAILED;

  // Note that strlen() returns the length of the string exclude the
  // terminating null character. The null value should be included
  // during transmission, so add one to the size.
  int stringSize = strlen(val) + 1;
  int status = this->packInt(stringSize);
  if (status == OK) {
  status = this->pack(val, stringSize);
  }
  return status;
  }
*/
int QUANTAnet_datapack_c::pack(char* val, int sz)
{
  if ((checkspace(CHAR_SIZE * sz) != OK) || (val == NULL))
    return FAILED;

  memcpy(running, val, CHAR_SIZE * sz);
  running += (CHAR_SIZE * sz);
  return OK;
}

#ifdef _WIN32_WCE
int QUANTAnet_datapack_c::pack(TCHAR* val, int sz)
{
  if ((checkspace(CHAR_SIZE * sz) != OK) || (val == NULL))
    return FAILED;

  WideCharToMultiByte(CP_ACP, 0, val, sz, (char *) running, sz, NULL, NULL);

  running += (CHAR_SIZE * sz);
  return OK;
}
#endif // _WIN32_WCE

//pack an array of floats
int QUANTAnet_datapack_c::packFloatArray(float* val, int sz)
{

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  FLOAT_VALUE* oldArray = new FLOAT_VALUE[sz];
  if (checkspace(FLOAT_SIZE * sz) != OK) return FAILED;
  //For each float, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= FLOAT_SIZE-1, j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, FLOAT_SIZE);
#endif
    running += (FLOAT_SIZE);

  }
  delete[] oldArray;
  return OK;
}

//pack an array of doubles
int QUANTAnet_datapack_c::packDoubleArray(double* val, int sz)
{

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  DOUBLE_VALUE* oldArray = new DOUBLE_VALUE[sz];
  if (checkspace(DOUBLE_SIZE * sz) != OK) return FAILED;

  //For each double, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= DOUBLE_SIZE-1, j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, DOUBLE_SIZE);
#endif
    running += (DOUBLE_SIZE);

  }
  delete[] oldArray;
  return OK;
}

//pack an array of int values
int QUANTAnet_datapack_c::packIntArray(int* val, int sz)
{

  INT_VALUE* oldArray = new INT_VALUE[sz];
  if (checkspace(INT_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  //For each int value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= INT_SIZE-1, j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, INT_SIZE);
#endif
    running += (INT_SIZE);

  }
  delete[] oldArray;
  return OK;
}

//pack an array of int32 values
int QUANTAnet_datapack_c::packInt32Array(int32* val, int sz)
{
  INT32_VALUE* oldArray = new INT32_VALUE[sz];
  if (checkspace(INT32_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  //For each int32 value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= INT32_SIZE-1,j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, INT32_SIZE);
#endif
    running += (INT32_SIZE);

  }
  delete[] oldArray;
  return OK;
}

//pack an array of int64 values
int QUANTAnet_datapack_c::packInt64Array(int64* val, int sz)
{
  INT64_VALUE* oldArray = new INT64_VALUE[sz];
  if (checkspace(INT64_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  //For each int64 value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= INT64_SIZE-1, j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, INT64_SIZE);
#endif
    running += (INT64_SIZE);

  }
  delete[] oldArray;
  return OK;
}

//pack an array of long values
int QUANTAnet_datapack_c::packLongArray(long* val, int sz)
{
  LONG_VALUE* oldArray = new LONG_VALUE[sz];
  if (checkspace(LONG_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
#endif

  //For each long value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
    oldArray[count].value = val[count];

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    for(i= LONG_SIZE-1, j=0; i>=0; i--, j++)
      running[j] = oldArray[count].bytes[i];
#else
    memcpy(running, oldArray[count].bytes, LONG_SIZE);
#endif
    running += (LONG_SIZE);

  }
  delete[] oldArray;
  return OK;
}

/// Float
int QUANTAnet_datapack_c::unpackFloat(float *Answer)
{
  FLOAT_VALUE newVal;

  if (checkspace(FLOAT_SIZE) != OK) return FAILED; // not enough buffer space

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[FLOAT_SIZE];

  memcpy(buffer, running, FLOAT_SIZE);

  // reverse the order to make the format from BigEndian to LittleEndian
  for (i = FLOAT_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, FLOAT_SIZE);
#endif // QUANTA_LITTLE_ENDIAN_FORMAT

  *Answer = newVal.value;
  running += FLOAT_SIZE; // increment the pointer
  return OK;
}

/// Int
int QUANTAnet_datapack_c::unpackInt(int *Answer)
{
  INT_VALUE newVal;

  if (checkspace(INT_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[INT_SIZE];

  memcpy(buffer, running, INT_SIZE);

  for (i = INT_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, INT_SIZE);
#endif

  *Answer = newVal.value;
  running += INT_SIZE;
  return OK;
}

/// Int32
int QUANTAnet_datapack_c::unpackInt32(int32 *Answer)
{
  INT32_VALUE newVal;

  if (checkspace(INT32_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[INT32_SIZE];

  memcpy(buffer, running, INT32_SIZE);

  for (i = INT32_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, INT32_SIZE);
#endif

  *Answer = newVal.value;
  running += INT32_SIZE;
  return OK;
}

/// Long
int QUANTAnet_datapack_c::unpackLong(long *Answer)
{
  LONG_VALUE newVal;

  if (checkspace(LONG_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[LONG_SIZE];

  memcpy(buffer, running, LONG_SIZE);

  for (i = LONG_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, LONG_SIZE);
#endif

  *Answer = newVal.value;
  running += LONG_SIZE;
  return OK;
}

/// Unsigned Long
int QUANTAnet_datapack_c::unpackUnsignedLong(unsigned long *Answer)
{
  UNSIGNED_LONG_VALUE newVal;

  if (checkspace(UNSIGNED_LONG_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[UNSIGNED_LONG_SIZE];

  memcpy(buffer, running, UNSIGNED_LONG_SIZE);

  for (i = UNSIGNED_LONG_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, UNSIGNED_LONG_SIZE);
#endif

  *Answer = newVal.value;
  running += UNSIGNED_LONG_SIZE;
  return OK;
}

int QUANTAnet_datapack_c::unpackInt64(int64 *Answer)
{
  INT64_VALUE newVal;

  if (checkspace(INT64_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[INT64_SIZE];

  memcpy(buffer, running, INT64_SIZE);

  for (i = INT64_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, INT64_SIZE);
#endif

  *Answer = newVal.value;
  running += INT64_SIZE;
  return OK;
}

/// Double
int QUANTAnet_datapack_c::unpackDouble(double  *Answer)
{
  DOUBLE_VALUE newVal;

  if (checkspace(DOUBLE_SIZE) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i, j;
  char buffer[DOUBLE_SIZE];

  memcpy(buffer, running, DOUBLE_SIZE);

  for (i = DOUBLE_SIZE - 1, j = 0; i >= 0; i--, j++)
    newVal.bytes[j] = buffer[i];
#else
  memcpy(newVal.bytes, running, DOUBLE_SIZE);
#endif

  *Answer = newVal.value;
  running += DOUBLE_SIZE;
  return OK;
}

/// Char
int QUANTAnet_datapack_c::unpackChar(char *Answer)
{
  if (checkspace(CHAR_SIZE) != OK) return FAILED;

  *Answer = *running++;
  return OK;
}


/// UNICODE Char
#ifdef _WIN32_WCE
int QUANTAnet_datapack_c::unpackChar(TCHAR *Answer)
{
  if (checkspace(CHAR_SIZE) != OK)
    return FAILED;

  MultiByteToWideChar(CP_ACP, 0, (char *) running, 1, Answer, 1);

  running++;
  return OK;
}
#endif // _WIN32_WCE
/*
  int QUANTAnet_datapack_c::unpackString(char* answer, const int& sz)
  {
  if (answer == NULL)
  return FAILED;

  int stringSize = 0;
  int status = this->unpackInt(&stringSize);
  if (status == OK) {
  // Check to make sure that the answer buffer is large enough to
  // unpack the string.
  if (sz >= stringSize) {
  status = this->unpack(answer, sz);
  } else {
  fprintf(stderr, "QUANTAnet_datapack_c::unpackString ERROR: "
  "not enough room to unpack the string!\n");
  status = FAILED;
  }
  }
  return status;
  }
*/
int QUANTAnet_datapack_c::unpack(char *Answer, int sz)
{
  if ((checkspace(CHAR_SIZE * sz) != OK) || (Answer == NULL))
    return FAILED;

  memcpy(Answer, running, CHAR_SIZE * sz);
  running += (CHAR_SIZE * sz);
  return OK;
}

#ifdef _WIN32_WCE
int QUANTAnet_datapack_c::unpack(TCHAR *Answer, int sz)
{
  if ((checkspace(CHAR_SIZE * sz) != OK) || (Answer == NULL))
    return FAILED;

  MultiByteToWideChar(CP_ACP, 0, (char *) running, sz, Answer, sz);

  running += (CHAR_SIZE * sz);
  return OK;
}
#endif // _WIN32_WCE

//unpack an array of floats
int QUANTAnet_datapack_c::unpackFloatArray(float* Answer, int sz)
{
  FLOAT_VALUE* newArray = new FLOAT_VALUE[sz];
  if (checkspace(FLOAT_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[FLOAT_SIZE];
#endif

  //For each float, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, FLOAT_SIZE);
    for(i= FLOAT_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, FLOAT_SIZE);
#endif
    running += (FLOAT_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

//unpack an array of doubles
int QUANTAnet_datapack_c::unpackDoubleArray(double* Answer, int sz)
{
  DOUBLE_VALUE* newArray = new DOUBLE_VALUE[sz];
  if (checkspace(DOUBLE_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[DOUBLE_SIZE];
#endif

  //For each value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, DOUBLE_SIZE);
    for(i= DOUBLE_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, DOUBLE_SIZE);
#endif
    running += (DOUBLE_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

//unpack an array of int values
int QUANTAnet_datapack_c::unpackIntArray(int* Answer, int sz)
{
  INT_VALUE* newArray = new INT_VALUE[sz];
  if (checkspace(INT_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[INT_SIZE];
#endif

  //For each value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, INT_SIZE);
    for(i= INT_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, INT_SIZE);
#endif
    running += (INT_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

//unpack an array of int32 values
int QUANTAnet_datapack_c::unpackInt32Array(int32* Answer, int sz)
{
  INT32_VALUE* newArray = new INT32_VALUE[sz];
  if (checkspace(INT32_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[INT32_SIZE];
#endif

  //For each value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, INT32_SIZE);
    for(i= INT32_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, INT32_SIZE);
#endif
    running += (INT32_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

//unpack an array of int64 values
int QUANTAnet_datapack_c::unpackInt64Array(int64* Answer, int sz)
{
  INT64_VALUE* newArray = new INT64_VALUE[sz];
  if (checkspace(INT64_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[INT64_SIZE];
#endif

  //For each value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, INT64_SIZE);
    for(i= INT64_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, INT64_SIZE);
#endif
    running += (INT64_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

//unpack an array of long values
int QUANTAnet_datapack_c::unpackLongArray(long* Answer, int sz)
{
  LONG_VALUE* newArray = new LONG_VALUE[sz];
  if (checkspace(LONG_SIZE * sz) != OK) return FAILED;

#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
  int i,j;
  char buffer[LONG_SIZE];
#endif

  //For each long value, copy the bytes and reverse the ordering for little endian conversion
  for (int count=0; count<sz; count++)
  {
#ifdef QUANTA_LITTLE_ENDIAN_FORMAT
    memcpy(buffer, running, LONG_SIZE);
    for(i= LONG_SIZE-1, j=0; i>=0; i--, j++)
      newArray[count].bytes[j] = buffer[i];
#else
    memcpy(newArray[count].bytes, running, LONG_SIZE);
#endif
    running += (LONG_SIZE);
    Answer[count] = newArray[count].value;
  }
  delete[] newArray;
  return OK;
}

int QUANTAnet_datapack_c::checkspace(unsigned int incoming_size)
{
  unsigned int remaining;

  remaining = start + size - running;

  if (remaining < incoming_size)
    return FAILED;
  else
    return OK;
}
