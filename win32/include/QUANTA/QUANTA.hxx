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

#ifndef _QUANTA_H
#define _QUANTA_H


#include <QUANTA/QUANTAmisc_observer_c.hxx>
#include <QUANTA/QUANTAmisc_debug.hxx>

#include <QUANTA/QUANTAinit.hxx>
#include <QUANTA/QUANTAts_mutex_c.hxx>
#include <QUANTA/QUANTAts_condition_c.hxx>
#include <QUANTA/QUANTAts_barrier_c.hxx>
#include <QUANTA/QUANTAts_thread_c.hxx>


#include <QUANTA/QUANTAnet_datapack_c.hxx>
#include <QUANTA/QUANTAnet_socketbase_c.hxx>
#include <QUANTA/QUANTAnet_tcp_c.hxx>
#include <QUANTA/QUANTAnet_udp_c.hxx>
#include <QUANTA/QUANTAnet_mcast_c.hxx>
#include <QUANTA/QUANTAnet_http_c.hxx>
#include <QUANTA/QUANTAnet_udpReflector_c.hxx>
#include <QUANTA/QUANTAnet_tcpReflector_c.hxx>
#include <QUANTA/QUANTAnet_remoteFileIO64_c.hxx>
#include <QUANTA/QUANTAnet_remoteFileIO32_c.hxx>
#include <QUANTA/QUANTAnet_rpc_c.hxx>
#include <QUANTA/QUANTAdb_c.hxx>
#include <QUANTA/QUANTAnet_parallelTcp_c.hxx>
#include <QUANTA/QUANTAnet_remoteParallelFileIO32_c.hxx>
#include <QUANTA/QUANTAnet_remoteParallelFileIO64_c.hxx>
#include <QUANTA/QUANTAnet_extendedParallelTcp_c.hxx>
#include <QUANTA/QUANTAnet_extendedTcp_c.hxx>
#include <QUANTA/QUANTAnet_barrierTcp_c.hxx>
#include <QUANTA/QUANTAnet_barrierUdp_c.hxx>

#include <QUANTA/QUANTAmisc_hash.hxx>
#include <QUANTA/QUANTAmisc_hashDict.hxx>

#include <QUANTA/QUANTAnet_objectStreamTcp_c.hxx>
#include <QUANTA/QUANTAnet_objectStreamUdp_c.hxx>


#ifdef WIN32
#include <QUANTA/gettimeofday.hxx>
#else
#ifdef WIN32
#include <QUANTA/QUANTAconfig_win32.hxx>
#else /* !WIN32 */
#include <QUANTA/QUANTAconfig.hxx>
#endif /* WIN32 */
#endif

#ifdef _WIN32_WCE
#include <time.h>
#endif

#endif
