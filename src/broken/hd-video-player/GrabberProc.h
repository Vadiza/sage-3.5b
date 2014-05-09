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
#ifndef _GRABBERCB_
#define _GRABBERCB_

#include <qedit.h>   // Sample grabber

#include "Histogram.h"
#include "Utils.h"
#include "output_sail.h"

class C_GrabProc {

  //  static GrabberCB m_grabberCallback;  // Make this global so that it stays in scope while the graph exists
  // create filter------------
  AM_MEDIA_TYPE m_mt;
  AM_MEDIA_TYPE m_audiomt;
  CComPtr<IBaseFilter> m_pGrabF;
  CComPtr<IBaseFilter> m_pAudioGrabF;
  CComPtr<IBaseFilter> m_pYUVtransF;
  int m_sub_type;
  // send image---------------
  BYTE*   m_pImg;
  BYTE*   m_pAudioSamples;
  DWORD   m_dwWidth;
  DWORD   m_dwHeight;
  LONG    m_lStride;
  int     m_iBitDepth;
  int   m_out_color_mode;
  unsigned char* m_pbRGB_buff;
  DWORD m_bufLen;
  bool m_sageOn;
  CString m_ip;

  class C_Output_sail Out_sail;

public :
  // create filter------------
  C_GrabProc(void);
  void FreeFilter(void);
  HRESULT SetFilter(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName, int out_color_mode, bool sage=false);
  HRESULT SetAudioFilter(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName);
  IBaseFilter* GrabF(void);
  // send image---------------
  HRESULT SetVideoFormat(const VIDEOINFOHEADER& vih, int out_color_mode);
  HRESULT SetImageBuffer(BYTE *pBuffer);
  HRESULT SetAudioBuffer(BYTE *pBuffer);
  HRESULT ConvertImage();
  int SendToBmp(void);
  int SendToSage(void);
  int SendAudioToSage(int buflen);
  void setIP(CString ip);

  void SetAudioOn(bool on);
  void SetAudioCaptureMode(bool on);
  void Shutdown();

  bool GetSageOn();
};

// GrabberCB: Class to implement the callback function for the sample grabber
// Note: No ref-counting, so keep the object in scope while you're using it!
class GrabberCB : public ISampleGrabberCB
{
private:
  BITMAPINFOHEADER m_bmi;  // Holds the bitmap format
  bool m_fFirstSample;     // true if the next sample is the first one.

public:

  GrabberCB();
  ~GrabberCB();

  C_GrabProc m_GrabProc;
  int m_out_color_mode;

  // IUnknown methods
  STDMETHODIMP_(ULONG) AddRef() { return 1; }
  STDMETHODIMP_(ULONG) Release() { return 2; }
  STDMETHOD(QueryInterface)(REFIID iid, void** ppv);

  // ISampleGrabberCB methods
  STDMETHOD(SampleCB)(double SampleTime, IMediaSample *pSample);
  STDMETHODIMP BufferCB(double, BYTE *, long) { return E_NOTIMPL; }

};

// AudioGrabberCB: Class to implement the callback function for the sample grabber
// Note: No ref-counting, so keep the object in scope while you're using it!
class AudioGrabberCB : public ISampleGrabberCB
{
private:
  BITMAPINFOHEADER m_bmi;  // Holds the bitmap format
  bool m_fFirstSample;     // true if the next sample is the first one.
  int m_index;
public:

  AudioGrabberCB();
  ~AudioGrabberCB();

  C_GrabProc* m_GrabProc;

  // IUnknown methods
  STDMETHODIMP_(ULONG) AddRef() { return 1; }
  STDMETHODIMP_(ULONG) Release() { return 2; }
  STDMETHOD(QueryInterface)(REFIID iid, void** ppv);

  // ISampleGrabberCB methods
  STDMETHOD(SampleCB)(double SampleTime, IMediaSample *pSample);
  STDMETHODIMP BufferCB(double, BYTE *, long) { return E_NOTIMPL; }

};

// GUID media_type_ary[] = {MEDIASUBTYPE_UYVY, MEDIASUBTYPE_YUY2, MEDIASUBTYPE_YUYV};

#ifdef _MAIN_
GrabberCB grabberCallback;  // Make this global so that it stays in scope while the graph exists
AudioGrabberCB AudioGrabCallback;
#else
extern GrabberCB grabberCallback;  // Make this global so that it stays in scope while the graph exists
extern AudioGrabberCB AudioGrabCallback;
#endif


#endif
