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

#ifndef _QUANTADB_SHAREDSTATE_C_HXX
#define _QUANTADB_SHAREDSTATE_C_HXX

#include <QUANTA/QUANTAmisc_observer_c.hxx>
#include <QUANTA/QUANTAnet_datapack_c.hxx>

// Forward declarations.
class QUANTAdb_client_c;

/** Encapsulates a shared state to be maintained between multiple users.
    This class provides a base abstract class from which to derives shared state
    information which multiple users encounter.  This state needs to be managed
    by a database client in order to maintain consistency, but the client itself
    is external to the scope of the state.  This state is associated with a
    particular path and key that should not change throughout the life of the
    state.

    Updates are handled through the user of the subject-observer pattern.  The
    shared state observes the client, so when new data arrives, it will unpack
    the state information provided that sharing is enabled.  Sharing is initially
    disabled for initialization purposes, but can be enabled and disabled later
    as required.

    Subclasses of the shared state have the responsiblity of handling the format
    of the state for transmission.  The state will need to be packed using the
    packer instance and then sent through the database client.  This process is
    handled by each client because the packing will vary, and the send is
    dependent upon the size of the packed data.  Unpacking state information,
    however, can be done with just the data itself.

    @author cavern@evl.uic.edu
    @version 03/26/2000
*/
class QUANTAdb_sharedState_c : public QUANTAmisc_observer_c
{
public:
  /// Class constructor.
  QUANTAdb_sharedState_c();

  /** Class constructor.
      Creating an instance of a shared state requires a reference to a
      database client which already exists.  The state will be registered
      with the client as an observer.  Additionally, path and key names
      are stored within the state for tranmitting and receiving state
      updates.
      @param dbclient pointer to an instantiated database client
      @param path path string to associate with this state
      @param key key string to associate with this state
  */
  QUANTAdb_sharedState_c( QUANTAdb_client_c *dbclient, char *path, char *key );
  /// Class destructor.
  virtual ~QUANTAdb_sharedState_c();
  /// Returns the sharing status of this state.
  inline bool isSharingEnabled() { return _sharingEnabled; }
  /// Sets the sharing enabled flag to TRUE.
  inline void enableSharing() { _sharingEnabled = true; }
  /// Sets the sharing enabled flag to FALSE.
  inline void disableSharing() { _sharingEnabled = false; }
  /** Requests a refresh of the data.
      This method requests the remote state information from the database.
      It may be useful to call after creating the state.
  */
  int refresh();
  virtual void update( QUANTAmisc_subject_c *subj );

protected:
  /** Packs the object state and transmits it to the database server.
      The state information must be packed into a data buffer and sent
      to the database server.  The responsibility of packing the state
      AND sending are placed within this method, so subclasses must
      put the state information into the database themselves.
      @return result Result of the attempt to send the state to the server
  */
  virtual int packAndSendState() { return 0; }
  /** Unpacks the object state from the data buffer.
      The state receives a data buffer from the client when an update
      has occurred, but the state information must be unpacked from
      that buffer.  This virtual method must be implemented in any
      derived subclass to extract the appropriate information from
      the data buffer received from the client.  It is highly advisable
      to check the data's path, key, and size for security.
      @param data character data buffer received from the server
      @param size size in bytes of data buffer
  */
  virtual int unpackState( char *data, const int& size = 0) { return 0; }

protected:
  /// data packing object
  QUANTAnet_datapack_c _packer;
  /// pointer to a database client which will make the transmissions
  QUANTAdb_client_c *_dbclient;
  /// flag for indicating whether or not sharing is enabled
  bool _sharingEnabled;
  /// path name in the database for storing this state
  char *_path;
  /// key name in the database for storing this state
  char *_key;
};

#endif
