/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
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
 * Direct questions, comments etc about SAGE to http://www.evl.uic.edu/cavern/forum/
 *
 *****************************************************************************/

//------------------------------------------------------------------------------------
// $Id: Utils.h,v 1.6 2006/03/21 07:00:57 ivanr Exp $
//
// Desc: DirectShow utility class header
//
// Copyright (c) Blackmagic Design 2005. All rights reserved.
//------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <dvdmedia.h>

//------------------------------------------------------------------------------------
// CDSUtils - Directshow utility class
//------------------------------------------------------------------------------------
class CDSUtils
{
public:
  static HRESULT AddFilter(IGraphBuilder* pGraph, const GUID &clsid, LPCWSTR pName, IBaseFilter** ppFilter);
  static HRESULT AddFilter2(IGraphBuilder* pGraph, const GUID &clsid, LPCWSTR pName, IBaseFilter** ppFilter);

  static HRESULT GetUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir, IPin** ppPin);
  static HRESULT GetPin(IBaseFilter* pFilter, const wchar_t* pName, IPin** ppPin);
  static HRESULT GetPin(IBaseFilter* pFilter, const GUID* pFormat, PIN_DIRECTION PinDir, IPin** ppPin);

  static HRESULT ConnectFilters(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName, IBaseFilter* pDownstream, wchar_t* pDownstreamPinName);
  static HRESULT ConnectFilters(IGraphBuilder* pGraph, IBaseFilter* pUpstream, IBaseFilter* pDownstream, const GUID* pFormat);
  static HRESULT RenderFilter(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName);

  static HRESULT DisconnectAllPins(IGraphBuilder* pGraph);

  static HRESULT FindFilterInterface(IBaseFilter* pFilter, const IID& riid, void** ppvInterface);
  static HRESULT FindPinInterface(IBaseFilter* pFilter, wchar_t* pName, const IID& riid, void** ppvInterface);
  static HRESULT FindPinInterface(IBaseFilter* pFilter, const GUID* pFormat, PIN_DIRECTION PinDir, const IID& riid, void** ppvInterface);

  static HRESULT AddGraphToRot(IUnknown* pUnkGraph, DWORD* pdwRegister);
  static void RemoveGraphFromRot(DWORD pdwRegister);
};

//------------------------------------------------------------------------------------
// CRegUtils - Registry utility class
//------------------------------------------------------------------------------------
class CRegUtils
{
public:
  CRegUtils();
  ~CRegUtils();

  LONG Open(LPCTSTR lpSubKey);
  LONG Create(LPCTSTR lpSubKey);
  void Close();

  LONG SetString(LPCTSTR valueName, const BYTE* lpData, DWORD cbData);
  LONG GetString(LPCTSTR valueName, LPBYTE lpData, DWORD cbData);

  LONG SetBinary(LPCTSTR valueName, const BYTE* lpData, DWORD cbData);
  LONG GetBinary(LPCTSTR valueName, LPBYTE lpData, DWORD cbData);

  LONG SetDword(LPCTSTR valueName, const BYTE* lpData, DWORD cbData);
  LONG GetDword(LPCTSTR valueName, LPBYTE lpData, DWORD cbData);

private:
  std::string m_subKeyName;
  HKEY m_hKey;
};

//------------------------------------------------------------------------------------
// CUtils - utility class
//------------------------------------------------------------------------------------
class CUtils
{
public:
  static BITMAPINFOHEADER* GetBMIHeader(const AM_MEDIA_TYPE* pamt);
  static BITMAPINFOHEADER* GetBMIHeader(const CMediaType& mt);
  static REFERENCE_TIME GetAvgTimePerFrame(const AM_MEDIA_TYPE* pamt);
  static unsigned long GetImageSize(BITMAPINFOHEADER* pbmih);
};

//------------------------------------------------------------------------------
// UNITS = 10 ^ 7
// UNITS / 30 = 30 fps;
// UNITS / 20 = 20 fps, etc
const REFERENCE_TIME FPS_30 = UNITS / 30;
const REFERENCE_TIME FPS_2997 = UNITS * 1001 / 30000;
const REFERENCE_TIME FPS_25 = UNITS / 25;
const REFERENCE_TIME FPS_24 = UNITS / 24;
const REFERENCE_TIME FPS_23976 = UNITS * 1001 / 24000;
const REFERENCE_TIME FPS_20 = UNITS / 20;
const REFERENCE_TIME FPS_10 = UNITS / 10;
const REFERENCE_TIME FPS_5  = UNITS / 5;
const REFERENCE_TIME FPS_4  = UNITS / 4;
const REFERENCE_TIME FPS_3  = UNITS / 3;
const REFERENCE_TIME FPS_2  = UNITS / 2;
const REFERENCE_TIME FPS_1  = UNITS / 1;

//------------------------------------------------------------------------------
#define SAFE_DELETE(p)      { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p)  { if(p) { delete [] (p);     (p)=NULL; } }
#define SAFE_RELEASE(p)     { if(p) { (p)->Release(); (p)=NULL; } }
#define SIZEOF_ARRAY(x)     (sizeof(x)/sizeof((x)[0]))
