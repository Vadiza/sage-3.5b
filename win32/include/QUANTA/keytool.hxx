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

#ifndef _KEYTOOL
#define _KEYTOOL

#include <QUANTA/QUANTAconfig.h>

#include <QUANTA/md5Key_c.hxx>
#include <QUANTA/QUANTAmisc_hashDict.hxx>

#ifdef QUANTA_THREAD_SAFE
#include <QUANTA/QUANTAts_mutex_c.hxx>
#endif

#ifdef QUANTA_THREAD_SAFE
#define _KEYTOOL_MANAGER_LOCK mutex->lock();
#define _KEYTOOL_MANAGER_UNLOCK mutex->unlock();
#define _KEYTOOL_ENTITY_LOCK  mutex->lock();
#define _KEYTOOL_ENTITY_UNLOCK  mutex->unlock();
#elif defined(NO_THREAD)
#define _KEYTOOL_MANAGER_LOCK
#define _KEYTOOL_MANAGER_UNLOCK
#define _KEYTOOL_ENTITY_LOCK
#define _KEYTOOL_ENTITY_UNLOCK
#else
#error Neither QUANTA_THREAD_SAFE nor NO_THREAD have been defined.
#endif


const int _kt_metaSize = 256;
const int _kt_idSize = 512;
const int _kt_nameSize = 512;

class keyEntity_c{
public:

  enum permission_lock_status_t {
    /// LOCKED
    LOCKED = 0,
    /// UNLOCKED
    UNLOCKED = 1,
    /// FAILED
    FAILED = 2
  };

  char *data;
  int userViewDataSize;
  int internalRealDataSize;

  char pathname[_kt_nameSize];
  char filename[_kt_nameSize];
  char permissionPassword[_kt_nameSize];
  permission_lock_status_t  permissionLockStatus;

  char *meta;
  int metaSize;

  md5Key_c keyId;
  double timeStamp;
  double auxTimeStamp;

  // Stamp the timestamp with the current time.
  void stampTime();

  // Stamp the aux timestamp with the current time.
  void stampAuxTime();

  // Set time stamp to some arbitrary time.
  void setTimeStamp(double theTime);

  // Set aux time stamp to some arbitrary time.
  void setAuxTimeStamp(double theTime);

  double  getTimeInSecs();


  // Set to 1 if this key did not exist in db file originally.
  // Used so that on commit we do not go through the whole
  // file renaming thing.
  int firstTimeKey;

  keyEntity_c ();

  // Added 2/26/99
  ~keyEntity_c() {
#ifdef QUANTA_THREAD_SAFE
    delete mutex;
#endif

    // 3/10/99
    if (data) delete data;
    if (meta) delete meta;

  }

  void lock() {
    _KEYTOOL_ENTITY_LOCK
      }

  void unlock() {
    _KEYTOOL_ENTITY_UNLOCK
      }


#ifdef QUANTA_THREAD_SAFE
  QUANTAts_mutex_c *mutex;
#endif


};

class keyToolManager_c;

/** Key tool key class. Objects of this type are created by the Key Tool Manager.
    @see keyToolManager_c
*/
class keyToolKey_c {
public:

  /// Status
  enum status_t {
    /// Failed.
    FAILED,
    /// Ok.
    OK,
    /// LOCKED
    LOCKED,
    /// ALREADY LOCKED
    ALREADY_LOCKED,
    /// UNLOCKED
    UNLOCKED,
    /// ALREADY  UNLOCKED
    ALREADY_UNLOCKED,
    /// CANNOT UNLOCK
    CANNOT_UNLOCK,
    /// CANNOT LOCK
    CANNOT_LOCK
  };

  /// Get the key id of the current key object.
  md5Key_c getKeyId();

  /// Compute the checksum of the data in the key.
  md5Key_c getDataCheckSum();

  /** Get pointer to data buffer in key.
      Need more elaborate call in future as in Stuart's original spec.
  */
  char *getData();

  /// Return size of data.
  int getDataSize();

  /// Return size of internal buffer.
  int getRealSize();

  /// Allocate memory in key (previous contents MAY be deleted).
  status_t alloc(int size);

  /// Delete the data in a key.
  status_t trash();

  /// Allocate memory while maintaining contents.
  /// status_t resize(int size);


  /// Commit this key (automatially timestamps. Does not affect aux time stamp.)
  status_t commit();

  status_t commit(char* password);

  /** Set meta field.
   */
  status_t setMeta(char* password, char *data, int msize);

  status_t setMeta(char *data, int msize);

  /** Lock a key.
      @param password is the "key" to the lock
      Returns FAILED, ALREADY_LOCKED (by you), CANNOT_LOCK, LOCKED (successfully)
  */
  status_t permissionLock(char *password);

  /** Unlock the key.
      Returns FAILED, ALREADY_UNLOCKED (by you), CANNOT_UNLOCK, UNLOCKED (successfully)
  */
  status_t permissionUnlock(char *password);

  /// Force an Unlock of the key.
  status_t permissionForceUnlock();

  /// Force a lock even when its locked by some other password.
  status_t permissionForceLock(char *password);

  /** Get the lock's status.
      @param password find out what password is holding the lock
      @param size of password char buffer to fill.
  */
  status_t getPermissionLockStatus(char *password, int size);


  /// Get meta data. Returns ptr to the buffer.

  char *getMeta();

  /// Return size of meta data.
  int getMetaSize();


  /// Force stamp the timestamp with the current time. Ie without doing a commit.
  void stampTime();

  /// Stamp the aux time stamp.
  void stampAuxTime();

  /// Set the time stamp to a particular time.
  void setTimeStamp(double theTime);

  /// Set the aux time stamp to a particular time.
  void setAuxTimeStamp(double theTime);

  /// Get the time stamp.
  double getTimeStamp();

  /// Get the aux time stamp.
  double getAuxTimeStamp();

private:
  md5Key_c itsKeyId;
  friend class keyToolManager_c;
  keyToolKey_c(keyToolManager_c *ktm);
  //  keyEntity_c *itsKeyEntity;
  keyToolManager_c *itsManager;
#ifndef WIN32 // Windows provides the same as a macro function
  int max(int s1, int s2) {if (s1 > s2) return s1; else return s2;}
#endif
};


/** Key Tool Manager class.
    This is a quick keytool simulator hack. It is a first try. The spec will
    no doubt change over time. So the documentation describes the functionality
    of the existing classes, which may or may not change in the final spec.

    To begin you need to create a key tool manager which basically sets up
    a directory in which is can store files to hold data for the keys
    (Ptool will no doubt have its own way of doing this). Currently every
    key gets 1 data file and 1 meta data file. The data file only contains
    the contents of the key. The meta data file contains the meta data:
    size of data, timestamp, comment.

    Using the keytool manager you can get a keytool key object (a wrapper)
    which gives you access to the internals of the keys- ie the actual
    data, the time stamp, etc. The keytool manager is essentially a cache
    between physical memory and secondary storage.

    Note, the difference between a time stamp and an aux (auxiliary) time stamp
    is that the former is always done on a commit. The latter is not.
    The aux time stamp is used to allow user defined timestamping. e.g.
    the aux time stamp can be used to record the timestamp of the original
    remote data source, whereas the regular timestamp can be used to record the
    time stamp of the local copy of the data.

*/
class keyToolManager_c {

public:
  keyToolManager_c(char *dir);
  ~keyToolManager_c();

  /// Status of getKey() call.
  enum status_t {
    /// Failed.
    FAILED = -1,
    /// Key already in manager.
    KEY_ALREADY_IN_MANAGER = 1,
    /// Key read in from file.
    KEY_FOUND_IN_FILE = 2,
    /// Key completely new- ie not in manager nor file.
    KEY_COMPLETELY_NEW = 3,
    /// Ok.
    OK  = 5,
    /// LOCKED
    LOCKED = 6
  };

  void lock() {
    _KEYTOOL_MANAGER_LOCK
      }

  void unlock() {
    _KEYTOOL_MANAGER_UNLOCK
      }

  /** Get key. If key does not exist already then a new
      key is generated in the manager. A wrapper (keyToolKey_c) is returned.
      This key does not get stored permanently until you do a commit.

      If key already exists in the database but not in the manager
      then load it from the database to the manager. Return a wrapper
      (keyToolKey_c) to the key.

      If key already exists in the manager then simply return a wrapper
      (keyToolKey_c) to the key.
  */
  keyToolKey_c *getKey(char *path, char *name, status_t *retStatus);

  /** Get key. This only returns a keyToolKey_c wrapper if the key
      has been previously loaded into the manager via the other overloading
      of the getKey call. If it hasn't then this call returns NULL.
  */
  keyToolKey_c *getKey(md5Key_c keyId, status_t *retStatus);

  /// Check to see if a key exists at all.
  int existsKey(char *path, char* name);

  /// Swap contents of keys including meta data (ptr switch). Only if permissions are available.

  /// Swap contents regardless of passwords.
  status_t swapContents(keyToolKey_c *thekey1, char* password1, keyToolKey_c *thekey2, char* password2);

  /// Swap contents of keys (ptr switch).
  status_t swapContents(keyToolKey_c *key1, keyToolKey_c *key2);

  /// Remove a key from this manager.
  status_t removeKey(keyToolKey_c *key);

  /// Remove a key from this manager and persistent store.
  status_t removeKeyPermanently(keyToolKey_c *key);

  /// Commit the entire database.
  status_t commit();

  // Commit an entry in the keyDB. NOT TO BE USED BY USER
  // CANNOT MAKE PRIVATE EITHER.
  status_t commit(keyEntity_c *);

  /// Show performance profile.
  void showProfile();

#ifdef PTHREAD_SAFE
  pthread_mutex_t mutex;
#endif

#ifdef QUANTA_THREAD_SAFE
  QUANTAts_mutex_c *mutex;
#endif


private:


  friend class keyToolKey_c;

  QUANTAmisc_hashDict <keyEntity_c *, md5Key_c> *keyDB;
  char rootdirectory[_kt_metaSize];


};

#endif
