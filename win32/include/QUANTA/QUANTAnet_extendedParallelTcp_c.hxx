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

#ifndef _QUANTAPLUS_EPARALLEL_TCP_C
#define _QUANTAPLUS_EPARALLEL_TCP_C

#include <QUANTA/QUANTAnet_parallelTcp_c.hxx>

/** This class is designed to help users minimize their effort of packing and
    unpacking numerical values every time they need to send one. All methods provided
    in this class will pack values before writing them to the network. Again, all methods
    available in this class will unpack all values gotten from the network before returning
    them to the users.
    This class is derived from QUANTAnet_parallelTcp_c class and thus, compatible with the class.
    So it is possible to use QUANTAnet_extendedParallelTcpClient_c in a machine and use
    QUANTAnet_parallelTcp_c class in the other, or vice versa; you are just losing the
    convenience of using simple methods provided in this class in return and you
    have to pack and unpack all numerical values by yourself. If you want to use this
    class in the server side, make sure to create an instance of
    QUANTAnet_extendedParallelTcpServer_c class instead of QUANTAnet_ParallelTcpServer_c
    since QUANTAnet_extendedParallelTcpServer_c gives you access to all methods
    available in this client class.
*/
class QUANTAnet_extendedParallelTcpClient_c : public QUANTAnet_parallelTcpClient_c
{
public:
  QUANTAnet_extendedParallelTcpClient_c();
  virtual ~QUANTAnet_extendedParallelTcpClient_c();

  /** reads a character
      @param value character you want to write to the network stream
  */
  int readChar(char& value);

  /** writes a character
      @param value character that is going to be read from the network stream
  */
  int writeChar(char value);

  /** reads an integer value
      @param value integer value you want to read from the network stream
  */
  int readInt(int& value);

  /** reads a number of integer values
      @param intArray array of integers to store the values read from network stream
      @param size the number of integers you want to send
  */
  int readIntArray(int* intArray, int size);

  /** writes an integer value
      @param value integer value that is going to be written to the network stream
  */
  int writeInt(int value);

  /** writes a number of integer values
      @param intArray integer values that will be sent to the network stream
      @param size the number of integers you want to send
  */
  int writeIntArray(int* intArray, int size);

  /** reads a 32-bit integer value
      @param value 32-bit integer value that will be read from the network stream
  */
  int readInt32(int32& value);

  /** reads a number of 32-bit integer values
      @param int32Array array of 32-bit integers to store the values read from network stream
      @param size the number of 32-bit integers you want to send
  */
  int readInt32Array(int32* int32Array, int size);

  /** writes a 32-bit integer value
      @param value 32-bit integer value that is going to be written to the network stream
  */
  int writeInt32(int32 value);

  /** writes a number of 32-bit integer values
      @param int32Array 32-bit integer values that will be sent to the network stream
      @param size the number of 32-bit integers you want to send
  */
  int writeInt32Array(int32* int32Array, int size);

  /** reads a 64-bit integer value
      @param value 64-bit integer value that will be read from the network stream
  */
  int readInt64(int64& value);

  /** reads a number of 64-bit integer values
      @param int64Array array of 64-bit integers to store the values read from network stream
      @param size the number of 64-bit integers you want to send
  */
  int readInt64Array(int64* int64Array, int size);

  /** writes a 64-bit integer value
      @param value 64-bit integer that is going to be written to the network stream
  */
  int writeInt64(int64 value);

  /** writes a number of 64-bit integer values
      @param int64Array 64-bit integer values that will be sent to the network stream
      @param size the number of 64-bit integers you want to send
  */
  int writeInt64Array(int64* int64Array, int size);

  /** reads a long integer value
      @param value long integer value you want to read from the network stream
  */
  int readLong(long& value);

  /** reads a number of long integer values
      @param longArray array of long integers to store the values read from network stream
      @param size the number of long integers you want to send
  */
  int readLongArray(long* longArray, int size);

  /** writes a long integer value
      @param value long integer value that is going to be written to the network stream
  */
  int writeLong(long value);

  /** writes a number of long integer values
      @param longArray long integer values that will be sent to the network stream
      @param size the number of long integers you want to send
  */
  int writeLongArray(long* longArray, int size);

  /** reads a single precision floating point value
      @param value float value you want to read from the network stream
  */
  int readFloat(float& value);

  /** reads a number of float values
      @param floatArray array of float values that store the values read from network stream
      @param size the number of float values you want to send
  */
  int readFloatArray(float* floatArray, int size);

  /** writes a float value
      @param value float value that is going to be written to the network stream
  */
  int writeFloat(float value);

  /** writes a number of float values
      @param floatArray float values that will be sent to the network stream
      @param size the number of float values you want to send
  */
  int writeFloatArray(float* floatArray, int size);

  /** reads a double precision floating point value
      @param value double value you want to read from the network stream
  */
  int readDouble(double& value);

  /** reads a number of double values
      @param doubleArray array of double values to store the values read from network stream
      @param size the number of double values you want to send
  */
  int readDoubleArray(double* doubleArray, int size);

  /** writes an integer value
      @param value double value that is going to be written to the network stream
  */
  int writeDouble(double value);

  /** writes a number of double values
      @param doubleArray double values that will be sent to the network stream
      @param size the number of double values you want to send
  */
  int writeDoubleArray(double* doubleArray, int size);

private:
  int int_size;
  int char_size;
  int int32_size;
  int int64_size;
  int long_size;
  int float_size;
  int double_size;

  char* int_buffer;
  char* int32_buffer;
  char* int64_buffer;
  char* long_buffer;
  char* float_buffer;
  char* double_buffer;
};

/** This class is derived from the parent class to return the extended client class.
 */
class QUANTAnet_extendedParallelTcpServer_c : public QUANTAnet_parallelTcpServer_c
{
public:
  /** Connect to a server.
      @returns QUANTAnet_parallelTcpServer_c::OK if connection is made
      successfully. Else returns QUANTAnet_parallelTcpServer_c::FAILED.
  */
  QUANTAnet_extendedParallelTcpClient_c *checkForNewConnections();
};

#endif
