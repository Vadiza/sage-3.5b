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

#ifdef HAVE_FTW_H
#ifndef _QUANTAMISC_FILELIST_C
#define _QUANTAMISC_FILELIST_C

#ifdef linux
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif


#ifndef __HAVE_STDIO_H
#include <stdio.h>
#define __HAVE_STDIO_H
#endif
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <QUANTA/QUANTAnet_datapack_c.hxx>

/**
   QUANTAmisc_fileInfo Class. This is a QUANTA class for storing information about files. It has utility functions used to store and pack information about the file name, the time stamp, the size of the file and a status flag to indicate if its a directory. This is used by the QUANTAmisc_fileList_c class to build file lists from remote servers
*/
class QUANTAmisc_fileInfo_c
{

protected:
  char* fileName;         //The file name with the full path
  long fileSize;          //The size of the file
  long timeStamp;         //Time stamp in seconds
  int objectType;         //To specify if its  file or a directory
  char* leafName;         //The leaf name of the file (without the full path)
  void setLeafName(char* path);
public:

  //@{
  /// Status ok
  static const int OK; /* =0 */
  /// Status failed
  static const int FAILED; /* = -1 */
  /// The Object type is a file
  static const int FILE_TYPE; /* = FTW_F */
  /// The Object type is a directory
  static const int DIRECTORY_TYPE; /* = FTW_D */
  /// The Object type is a directory which does not have read permissisons
  static const int DIRECTORY_TYPE_CANNOT_BE_READ; /* = FTW_DNR */
  //@}

  QUANTAmisc_fileInfo_c();
  ~QUANTAmisc_fileInfo_c();

  /**
     Set the file information - the filename, file size, time stamp and type
     @param path Path name on the remote machine
     @param fName File name or directory name
     @param fSize File Size
     @param tStamp Time stamp (a long value giving the # seconds)
     @param type Object type to indicate if its a file or directory - the accepted values are DIRECTORY_TYPE, FILE_TYPE, DIRECTORY_TYPE_CANNOT_BE_READ
  */
  void setFileInfo(char* path, char* fName, long fSize, long tStamp, int type);

  /**
     Get the file name with the path
     @param fName
     File name handle. Do note that memory is allocated for fName inside this function and the programmer need not allocate memory
  */
  void getFileName(char*& fName);

  /**
     Get the absolute file name, without the path
     @param leafName
     Handle to the leaf name.  Do note that memory is allocated for lName inside this function and the programmer need not allocate memory
  */
  void getLeafName(char*& lName);


  /**
     Get the file size
     @return fileSize
     Size of the file in bytes
  */
  long getFileSize();

  /**
     Get the time stamp
     @return timeStamp
     time stamp of the file (# seconds)
  */
  long getTimeStamp();

  /**
     Get the object type
     @return FILE_TYPE, DIRECTORY_TYPE, DIRECTORY_TYPE_CANNOT_BE_READ
  */
  int getObjectType();

  /**
     Get the packing size
     @return packingSize
     The number of bytes required to pack this object
  */
  int getPackingSize();

  /**
     Pack the file info in a buffer, specified by a pointer to a datapacking object. Before calling this function, the datapacking size should be obtained and the data packer instance should be initialized (using the initpack function)
     @param packer Pointer to the QUANTAnet_datapack_c class
     @return OK or FAILED
  */
  int packFileInfo(QUANTAnet_datapack_c* packer);

  /**
     Unpack the file info from a buffer, specified by a pointer to a datapacking object. Before calling this function, the data unpacking size should be obtained and the data packer instance should be initialized (using the initUnpack function)
     @param packer Pointer to the QUANTAnet_datapack_c class
     @param path Path name form which the list of files is got
     @return OK or FAILED
  */
  int unpackFileInfo(QUANTAnet_datapack_c* packer, char* path);

  /**
     Display the file info in the standard long format
  */
  void displayFileInfo();
};

/**
   QUANTA class for building a list of  files in a directory. This is used by the remote fileIO classes and the web client class.
*/

class QUANTAmisc_fileList_c
{
private:
  int depth;
  int I_ALLOCATED_MEMORY;  //used to check what memory to deallocate
  static int ftwCallBack(const char* fname, const struct stat* statBuf, int objectType, struct FTW* ftwstruct);
  static int ftwDummyCallBack(const char* fname, const struct stat* statBuf, int objectType, struct FTW* ftwstruct);
  static QUANTAmisc_fileList_c* thisInstance ;
public:
  //@{
  /// Status ok
  static const int OK; /* =0 */
  /// Status failed
  static const int FAILED; /* = -1 */
  /// List the files recursively
  static const int LIST_RECURSIVELY; /* = 2 */
  /// Skip recursion
  static const int SKIP_RECURSION; /* = 3 */
  /// Traverse links, while listing files
  static const int TRAVERSE_LINKS; /* = 4 */
  /// Do not traverse links, while listing files
  static const int DO_NOT_TRAVERSE_LINKS; /* = 5 */
  //@}

  char* path;
  int noFiles;
  int fileCount;
  int doRecursion;
  QUANTAmisc_fileInfo_c* fileList;

  QUANTAmisc_fileList_c();
  ~QUANTAmisc_fileList_c();

  /**
     Set the depth for a file tree walk - the depth should be atleast as long as the depth of the last subdirectory for a fast response
     @param newDepth the depth value (default value = 25)
  */
  void setDepth(int newDepth);

  /**
     Build the file list in a given path
     Note:
     1. Symbolic links would not be traversed
     2. The system call used to obtain the list of files forced the use of static variables. So the user should take care of mutual exclusion problems -it should be made sure that not more than one process/thread uses the buildFileList() API at the same time

     @param dirName The name of the path
     @param recursion Flag to specify whether a recursive list of files is required -accepted values are SKIP_RECURSION and LIST_RECURSIVELY
     @param linkTraversal Flag to specify whether links have to be traversed (by default it is not)- accepted values are TRAVERSE_LINKS and DO_NOT_TRAVERSE_LINKS
     @return OK or FAILED
  */
  int buildFileList(char* dirName, int recursion = SKIP_RECURSION, int linkTraversal = DO_NOT_TRAVERSE_LINKS);

  /**
     Get the number of files in the list
     @return noFiles
     The number of files
  */
  int getNoFiles();

  /**
     Pack the file list in a buffer, specified by a pointer to a datapacking object. Before calling this function, the datapacking size should be obtained and the data packer instance should be initialized (using the initpack function)
     @param packer Pointer to the QUANTAnet_datapack_c class
     @return OK or FAILED
  */
  int packFileList(QUANTAnet_datapack_c* packer);

  /**
     Obtain the data packing size
     @return packingSize
     The total datapacking size to pack information of all the files in the file list
  */
  int getDataPackingSize();

  /**
     Un pack the file list in a buffer, specified by a pointer to a datapacking object. Before calling this function, the datapacking size should be obtained and the data packer instance should be initialized (using the initUnpack function)
     @param packer Pointer to the QUANTAnet_datapack_c class
     @param path Remote path name
     @param list Pointer to the QUANTAmisc_fileInfo_c objects
     @param noFiles The number of files in the list
     @return OK or FAILED
  */
  int unpackfileList(QUANTAnet_datapack_c* packer, char* path, QUANTAmisc_fileInfo_c* list, int fileCount);

  /**
     Display the file list in the long standard format
  */
  void displayFileList();
};

#endif
#endif /* HAVE_FTW_H */
