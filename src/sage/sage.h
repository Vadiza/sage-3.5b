/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sage.h
 * Author : Byungil Jeong
 *
 * Copyright (C) 2004 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc about SAGE to bijeong@evl.uic.edu or
 * http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

#ifndef _SAGE_H
#define _SAGE_H

#include "sageBase.h"

#define SAGE_VERSION "3.0"
#define SAGE_BLOCK_NO_SYNC  100
#define SAGE_BLOCK_CONST_SYNC  101
#define SAGE_BLOCK_SOFT_SYNC   102
#define SAGE_BLOCK_HARD_SYNC   103

// Message Start Point
#define FS_CORE_MESSAGE 0
#define SAGE_UI_TO_FSM 1000
#define APP_UI_TO_FSM 2000
#define APP_TO_FSM 3000
#define DISP_MESSAGE 10000
#define GRCV_MESSAGE 20000
#define SAIL_MESSAGE 30000
#define APP_MESSAGE 31000
#define FSM_TO_SAGE_UI 40000
#define FSM_TO_APP_UI  41000
#define BRIDGE_MESSAGE 50000
#define ARCV_MESSAGE 15000

// fsManager Messages
#define REG_APP  FS_CORE_MESSAGE + 1  // app_name block_width block_height pixel_size sync_mode
#define REG_GRCV FS_CORE_MESSAGE + 2
#define FS_APPDISP_KILLED  FS_CORE_MESSAGE + 3
#define FS_TIME_MSG FS_CORE_MESSAGE + 4
#define REG_ARCV FS_CORE_MESSAGE + 5
#define SYNC_INIT_ARCV FS_CORE_MESSAGE + 6
#define NOTIFY_APP_SHUTDOWN    FS_CORE_MESSAGE + 7

// UI to fsManager
#define SAGE_UI_REG   SAGE_UI_TO_FSM
#define EXEC_APP      SAGE_UI_TO_FSM + 1
#define SHUTDOWN_APP  SAGE_UI_TO_FSM + 2
#define MOVE_WINDOW   SAGE_UI_TO_FSM + 3
#define RESIZE_WINDOW   SAGE_UI_TO_FSM + 4
#define PERF_INFO_REQ   SAGE_UI_TO_FSM + 5
#define STOP_PERF_INFO  SAGE_UI_TO_FSM + 6
#define SAGE_BG_COLOR   SAGE_UI_TO_FSM + 7
#define SAGE_Z_VALUE      SAGE_UI_TO_FSM + 8
#define SAGE_ADMIN_CHECK  SAGE_UI_TO_FSM + 9
#define BRING_TO_FRONT    SAGE_UI_TO_FSM + 10
#define UPDATE_WIN_PROP   SAGE_UI_TO_FSM + 11
#define APP_FRAME_RATE    SAGE_UI_TO_FSM + 12
#define APP_PAUSE_INTV    SAGE_UI_TO_FSM + 13
#define SAGE_APP_SHARE    SAGE_UI_TO_FSM + 14
#define SAGE_FLIP_WINDOW   SAGE_UI_TO_FSM + 15
#define SAGE_CHECK_LATENCY SAGE_UI_TO_FSM + 17
#define ROTATE_WINDOW      SAGE_UI_TO_FSM + 18
#define PUSH_TO_BACK       SAGE_UI_TO_FSM + 19
#define SAVE_SCREENSHOT    SAGE_UI_TO_FSM + 20

#define NETWORK_RESERVED   SAGE_UI_TO_FSM + 50

#define SAGE_SHUTDOWN    SAGE_UI_TO_FSM + 100
#define STREAM_PAUSE     SAGE_UI_TO_FSM + 101
#define STREAM_RESUME    SAGE_UI_TO_FSM + 102

#define ADD_OBJECT      SAGE_UI_TO_FSM + 200
#define MOVE_OBJECT     SAGE_UI_TO_FSM + 201
#define REMOVE_OBJECT   SAGE_UI_TO_FSM + 202
#define OBJECT_MESSAGE  SAGE_UI_TO_FSM + 203
#define SHOW_OBJECT     SAGE_UI_TO_FSM + 204

#define APP_UI_REG   APP_UI_TO_FSM

// displayInstance Messages from SAIL
#define DISP_STREAM_INFO      DISP_MESSAGE
#define DISP_SAIL_RESUME       DISP_MESSAGE + 4
#define DISP_SAIL_PERF_RPT     DISP_MESSAGE + 5

// displayInstance Messages from SAGE Receiver
#define DISP_START_STREAM      DISP_MESSAGE + 100
#define DISP_APP_CONNECTED     DISP_MESSAGE + 101
#define DISP_DEPTH_CHANGED     DISP_MESSAGE + 103
#define DISP_RCV_FRATE_RPT     DISP_MESSAGE + 104
#define DISP_RCV_BANDWITH_RPT  DISP_MESSAGE + 105

// gStreamRcv Messages
#define RCV_INIT               GRCV_MESSAGE
#define RCV_UPDATE_DISPLAY     GRCV_MESSAGE + 1
#define RCV_CLEAR_DISPLAY      GRCV_MESSAGE + 2
#define SHUTDOWN_RECEIVERS     GRCV_MESSAGE + 3
#define RCV_CHANGE_BGCOLOR     GRCV_MESSAGE + 4
#define RCV_SHUTDOWN_APP       GRCV_MESSAGE + 5
#define RCV_CHANGE_DEPTH       GRCV_MESSAGE + 6
#define RCV_PERF_INFO_REQ      GRCV_MESSAGE + 7
#define RCV_PERF_INFO_STOP     GRCV_MESSAGE + 8
#define UPDATE_OBJECT_POSITION GRCV_MESSAGE + 9
#define RCV_CHANGE_APP_BOUNDS  GRCV_MESSAGE + 10

// aStreamRcv Messages
#define ARCV_AUDIO_INIT        ARCV_MESSAGE
#define ARCV_SYNC_INIT         ARCV_MESSAGE + 1
#define ARCV_SHUTDOWN_APP      ARCV_MESSAGE + 2

// sage bridge messages
#define BRIDGE_REG_NODE        BRIDGE_MESSAGE
#define BRIDGE_SLAVE_INIT      BRIDGE_MESSAGE + 1
#define BRIDGE_SHUTDOWN        BRIDGE_MESSAGE + 2
#define BRIDGE_SHUTDOWN_APP    BRIDGE_MESSAGE + 3
#define BRIDGE_SLAVE_PERF      BRIDGE_MESSAGE + 4
#define BRIDGE_SYNC_ID         BRIDGE_MESSAGE + 5
#define BRIDGE_APP_REG         BRIDGE_MESSAGE + 6
#define BRIDGE_SLAVE_READY     BRIDGE_MESSAGE + 7
#define BRIDGE_STREAM_INIT     BRIDGE_MESSAGE + 8
#define BRIDGE_UI_REG          BRIDGE_MESSAGE + 9
#define BRIDGE_APP_INST_READY BRIDGE_MESSAGE + 10
#define CLEAR_APP_INSTANCE    BRIDGE_MESSAGE + 11

// fsManager to UI : 40000
#define SAGE_STATUS      FSM_TO_SAGE_UI
#define APP_INFO_RETURN  FSM_TO_SAGE_UI + 1
#define UI_PERF_INFO     FSM_TO_SAGE_UI + 2
#define UI_APP_SHUTDOWN    FSM_TO_SAGE_UI + 3
#define SAGE_DISPLAY_INFO  FSM_TO_SAGE_UI + 4
#define Z_VALUE_RETURN     FSM_TO_SAGE_UI + 5
#define APP_EXEC_CONFIG    FSM_TO_SAGE_UI + 6
#define DISP_CONNECTION_INFO  FSM_TO_SAGE_UI + 7
#define UI_ADMIN_INFO         FSM_TO_SAGE_UI + 8
#define UI_MOVE_FAIL          FSM_TO_SAGE_UI + 15
#define UI_RESIZE_FAIL        FSM_TO_SAGE_UI + 16
#define UI_PERF_REQ_FAIL      FSM_TO_SAGE_UI + 17
#define UI_OBJECT_INFO        FSM_TO_SAGE_UI + 18
#define UI_OBJECT_REMOVED     FSM_TO_SAGE_UI + 19
#define APP_OBJECT_CHANGE     FSM_TO_SAGE_UI + 20

#define REQUEST_BANDWIDTH  FSM_TO_SAGE_UI + 100

#define POINTER_STATUS     FSM_TO_SAGE_UI + 200

#define APP_STATUS         FSM_TO_APP_UI

// SAIL Messages
#define SAIL_UI_CLIENT        SAIL_MESSAGE + 1
#define SAIL_SLAVE_REG        SAIL_MESSAGE + 2
#define SAIL_SHUTDOWN         SAIL_MESSAGE + 3
#define SAIL_STREAM_START     SAIL_MESSAGE + 4
#define SAIL_CONNECTED_TO_RCV  SAIL_MESSAGE + 5
#define SAIL_PERF_INFO         SAIL_MESSAGE + 6
#define SAIL_SEND_TIME_BLOCK   SAIL_MESSAGE + 7
#define SAIL_CONFIG_STREAM     SAIL_MESSAGE + 8
#define SAIL_INIT_STREAM       SAIL_MESSAGE + 9

#define SAIL_CONNECT_TO_RCV  SAIL_MESSAGE + 101
#define SAIL_INIT_MSG        SAIL_MESSAGE + 102
#define SAIL_PERF_INFO_REQ   SAIL_MESSAGE + 103
#define SAIL_PERF_INFO_STOP  SAIL_MESSAGE + 104
#define SAIL_FRAME_RATE      SAIL_MESSAGE + 105
#define SAIL_FLIP_WINDOW     SAIL_MESSAGE + 106
#define SAIL_CONNECT_TO_ARCV SAIL_MESSAGE + 107
#define SAIL_RESEND_FRAME    SAIL_MESSAGE + 108
#define SAIL_CONNECT_TO_RCV_PORT SAIL_MESSAGE + 109

// App UI to APP
#define EVT_CLICK            APP_MESSAGE
#define EVT_DOUBLE_CLICK     APP_MESSAGE + 1
#define EVT_MOVE             APP_MESSAGE + 2
#define EVT_ANALOG1          APP_MESSAGE + 3
#define EVT_PAN              APP_MESSAGE + 3
#define EVT_ANALOG2          APP_MESSAGE + 4
#define EVT_ROTATE           APP_MESSAGE + 4
#define EVT_ANALOG3          APP_MESSAGE + 5
#define EVT_ZOOM             APP_MESSAGE + 5
#define EVT_ARROW            APP_MESSAGE + 6
#define EVT_KEY              APP_MESSAGE + 7
#define EVT_APP_SHARE        APP_MESSAGE + 11   // triggered by 'share with' button
#define EVT_APP_SYNC         APP_MESSAGE + 12   // sync app when sharing

#define EVT_APP_WIDGET       APP_MESSAGE + 100
#define APP_MINIMIZED        APP_MESSAGE + 101


// FSM to APP
#define APP_QUIT             APP_MESSAGE + 2000
#define APP_REFRESH_FRAME    APP_MESSAGE + 2001

#if defined(WIN32)
// Conversion from 'size_t' to 'int'
#pragma warning (disable: 4267)  //disable warning 4267
// Nonstandard extension used : zero-sized array in struct/union
#pragma warning (disable: 4200)  //disable warning 4267
#endif

#endif
