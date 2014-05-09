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

/*
 * File name: QUANTAnet_iovec.cxx
 *
 * Description: Wrapper class for the iovec structure
 *
 * Comments:
 *
 * Change Log:
 */

#include "QUANTAnet_iovec_c.hxx"

//constructor
QUANTAnet_iovec_c :: QUANTAnet_iovec_c(int size)
{
  num_of_elements = 0;
  nalloc = size;
  bufList = NULL;
}

//destructor
QUANTAnet_iovec_c :: ~QUANTAnet_iovec_c()
{

  if (bufList) {
    free(bufList);
    bufList = NULL;
  }

}

//add pointer and its length to the array
bool QUANTAnet_iovec_c :: add(void* buff, int length)
{
  if(bufList == NULL)
  {
#ifdef WIN32
    bufList = (WSABUF *) malloc (sizeof (WSABUF) * nalloc);
#else
    bufList = (struct iovec*) malloc (sizeof (iovec) * nalloc);
#endif

    //if malloc can not allocate memory
    if(bufList == NULL)
    {
      printf("QUANTAnet_iovec_c :: add() :: malloc - out of memory");
      return false;
    }
  }

  else if(num_of_elements >= nalloc)
  {
    nalloc = nalloc*2; //nalloc is doubled to reallocate twice number of elements
#ifdef WIN32
    bufList = (WSABUF *)realloc((WSABUF *)bufList, nalloc * sizeof(WSABUF));
#else
    bufList = (struct iovec*)realloc((iovec *)bufList, nalloc * sizeof(iovec));
#endif

    //if realloc can not allocate memory
    if(bufList == NULL)
    {
      printf("QUANTAnet_iovec_c :: add() :: realloc - out of memory");
      return false;
    }
  }

#ifdef WIN32
  bufList[num_of_elements].buf = (char*)buff;
  bufList[num_of_elements].len = length;
#else
  bufList[num_of_elements].iov_base = (char *)buff;
  bufList[num_of_elements].iov_len = length;
#endif

  ++num_of_elements;
  return true;
}

//remove the entry at index
bool QUANTAnet_iovec_c :: removeAt(int index)
{
  --num_of_elements;

  if(index > num_of_elements)
    return false;
  else
  {
    for(int i = index; i < num_of_elements; i++)
      bufList[i] = bufList[i + 1];
    return true;
  }
}

//clear the array
void QUANTAnet_iovec_c :: removeAll()
{
  num_of_elements = 0;
  bufList = NULL;

}
//returns the size fo teh array
int QUANTAnet_iovec_c :: size()
{
  return num_of_elements;
}
