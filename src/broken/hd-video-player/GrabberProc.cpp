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

#include "StdAfx.h"
#include <aviriff.h>  // defines 'FCC' macro

#include "math.h"
#include "YUV_RGB.h"

#include "GrabberProc.h"
#include "Utils2.h"

//------------------------------------
// If you want to test with saving BMP files, delete this define.
#define _SAGE_
//------------------------------------

//-------------------------------------------------------------------------------------------
//=================================[ class GrabberCB ]=======================================
//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
GrabberCB::GrabberCB() : m_fFirstSample(true)
{
}

//-------------------------------------------------------------------------------------------
GrabberCB::~GrabberCB()
{
}

//-------------------------------------------------------------------------------------------
// Support querying for ISampleGrabberCB interface
HRESULT GrabberCB::QueryInterface(REFIID iid, void**ppv)
{
  if (!ppv) { return E_POINTER; }
  if (iid == IID_IUnknown)
  {
    *ppv = static_cast<IUnknown*>(this);
  }
  else if (iid == IID_ISampleGrabberCB)
  {
    *ppv = static_cast<ISampleGrabberCB*>(this);
  }
  else
  {
    return E_NOINTERFACE;
  }
  AddRef();  // We don't actually ref count, but in case we change the implementation later.
  return S_OK;
}

//-------------------------------------------------------------------------------------------
// SampleCB: This is where we process each sample
HRESULT GrabberCB::SampleCB(double SampleTime, IMediaSample *pSample)
{
  static int init_flag = 0;
  /*
    FILE *fp;

    fp = fopen("./log.dat", "w");
    fprintf(fp, "\r\nin sampleCB");
    fclose(fp);
  */

  HRESULT hr;

  // Get the pointer to the buffer
  BYTE *pBuffer;
  hr = pSample->GetPointer(&pBuffer);

  // This funtion must be called every time
  m_GrabProc.SetImageBuffer(pBuffer);

  if (FAILED(hr))
  {
    OutputDebugString(TEXT("SampleCB: GetPointer FAILED\n"));
    return hr;
  }

  // Scan the image on the first sample. Re-scan is there is a discontinuity.
  // (This will produce horrible results if there are big scene changes in the
  // video that are not associated with discontinuities. Might be safer to re-scan
  // each image, at a higher perf cost.)

#ifdef _SAGE_
  // RGB -> Send
  //if(m_GrabProc.GetSageOn() == true) {
  hr = m_GrabProc.SendToSage();
  //}
#else
  // Convert the image
  // RGB -> Save
  hr = m_GrabProc.SendToBmp();
  // YUV -> RGB -> Send or Save
  //  hr = m_GrabProc.ConvertImage();
#endif

  if(FAILED(hr)) {
    exit(1);
  }

  return hr;
}

//-------------------------------------------------------------------------------------------
//=================================[ class AudioGrabberCB ]=======================================
//-------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------
AudioGrabberCB::AudioGrabberCB() : m_fFirstSample(true), m_index(0)
{
  m_GrabProc = NULL;
}

//-------------------------------------------------------------------------------------------
AudioGrabberCB::~AudioGrabberCB()
{
}

//-------------------------------------------------------------------------------------------
// Support querying for ISampleGrabberCB interface
HRESULT AudioGrabberCB::QueryInterface(REFIID iid, void**ppv)
{
  if (!ppv) { return E_POINTER; }
  if (iid == IID_IUnknown)
  {
    *ppv = static_cast<IUnknown*>(this);
  }
  else if (iid == IID_ISampleGrabberCB)
  {
    *ppv = static_cast<ISampleGrabberCB*>(this);
  }
  else
  {
    return E_NOINTERFACE;
  }
  AddRef();  // We don't actually ref count, but in case we change the implementation later.
  return S_OK;
}

//-------------------------------------------------------------------------------------------
// SampleCB: This is where we process each sample
HRESULT AudioGrabberCB::SampleCB(double SampleTime, IMediaSample *pSample)
{

  HRESULT hr;

  // Get the pointer to the buffer
  BYTE *pBuffer;
  hr = pSample->GetPointer(&pBuffer);

  /////// for the test
  int lenBuf = pSample->GetActualDataLength();

  // This funtion must be called every time
  m_GrabProc->SetAudioBuffer(pBuffer);

  if (FAILED(hr))
  {
    OutputDebugString(TEXT("SampleCB: GetPointer FAILED\n"));
    return hr;
  }

  // Scan the image on the first sample. Re-scan is there is a discontinuity.
  // (This will produce horrible results if there are big scene changes in the
  // video that are not associated with discontinuities. Might be safer to re-scan
  // each image, at a higher perf cost.)

#ifdef _SAGE_
  // RGB -> Send
  //if(m_GrabProc->GetSageOn() == true) {
  hr = m_GrabProc->SendAudioToSage(lenBuf);
  //}
#else

#endif

  if(FAILED(hr)) {
    exit(1);
  }

  return hr;
}


//-------------------------------------------------------------------------------------------
//=================================[ class GrabProc ]=======================================
//-------------------------------------------------------------------------------------------

// #define _YUVtransF_

#define MAX_SUB_TYPE  4
GUID media_type_ary[] = {MEDIASUBTYPE_RGB24, MEDIASUBTYPE_UYVY, MEDIASUBTYPE_YUY2, MEDIASUBTYPE_YUYV};

// {A6512C9F-A47B-45ba-A054-0DB0D4BB87F7}
//static const GUID CLSID_YuvGray =
//{ 0xa6512c9f, 0xa47b, 0x45ba, { 0xa0, 0x54, 0xd, 0xb0, 0xd4, 0xbb, 0x87, 0xf7 } };
//{ 0xB179A682, 0x641B, 0x11D2, { 0xA4, 0xD9, 0x00, 0x60, 0x08, 0x0B, 0xA6, 0x34} };

//{B179A682-641B-11D2-A4D9-0060080BA634}
#ifdef _YUVtransF_
static const GUID CLSID_YUVxfm =
  { 0xB179A682, 0x641B, 0x11D2, { 0xA4, 0xD9, 0x00, 0x60, 0x08, 0x0B, 0xA6, 0x34} };
#endif

//{ 0xa6512c9f, 0xa47b, 0x45ba, { 0xa0, 0x54, 0xd, 0xb0, 0xd4, 0xbb, 0x87, 0xf7 } };
// {B179A682-641B-11D2-A4D9-0060080BA634}
//DEFINE_GUID(CLSID_YUVxfm,
//0xB179A682, 0x641B, 0x11D2, 0xA4, 0xD9, 0x00, 0x60, 0x08, 0x0B, 0xA6, 0x34);
// 0x2fa4f053, 0x6d60, 0x4cb0, 0x95, 0x3, 0x8e, 0x89, 0x23, 0x4f, 0x3f, 0x73);

//-------------------------------------------------------------------------------------------
C_GrabProc::C_GrabProc(void) {
  m_pGrabF = NULL;
  m_pAudioGrabF = NULL;
  m_pYUVtransF = NULL;

  m_dwWidth = m_dwHeight = m_lStride = m_iBitDepth;
  m_pImg = NULL;
  m_pAudioSamples = NULL;
  m_pbRGB_buff = NULL;
  m_sageOn = false;
}


//-------------------------------------------------------------------------------------------
void C_GrabProc::FreeFilter(void) {
  //  m_pGrabF->Release();

  ///*
  if(m_pGrabF) {
    grabberCallback.Release();

#ifdef _YUVtransF_
    delete m_pYUVtransF;
    m_pYUVtransF = NULL;
#endif

    // Don't use delete with filter. Delete is ungraceful
    //    delete m_pGrabF;
    m_pGrabF = NULL;
  }
  // */
  if(m_pbRGB_buff) {
    free(m_pbRGB_buff);
    m_pbRGB_buff = NULL;
  }
}

HRESULT C_GrabProc::SetFilter(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName, int out_color_mode, bool sage) {
  HRESULT hr;

  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"Before Add filter.GRF"));

  m_sageOn = sage;
#ifdef _YUVtransF_
  hr = CDSUtils::AddFilter(pGraph, CLSID_YUVxfm, L"YUV Transform", &m_pYUVtransF);
#endif

  if(m_pGrabF == NULL) {
    hr = CDSUtils::AddFilter(pGraph, CLSID_SampleGrabber, L"Grabber", &m_pGrabF);
    if(hr < 0) {
      //SaveLog.ToLog("\n Can not Add filter : Grabber.");
      return hr;
    }
    //SaveLog.ToLog("\n Grabber is added ");
    //SaveLog.ToLog((int)&m_pGrabF);
  }
  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"Before connect filter.GRF"));

  CComQIPtr<ISampleGrabber> pGrabber(m_pGrabF);

  //  if(++m_sub_type > MAX_SUB_TYPE) return -1;

  // Configure the sample grabber
  ZeroMemory(&m_mt, sizeof(AM_MEDIA_TYPE));
  m_mt.majortype = MEDIATYPE_Video;
  m_mt.formattype = FORMAT_VideoInfo;

  if(out_color_mode == RGB24) m_mt.subtype = MEDIASUBTYPE_RGB24;
  else if(out_color_mode == RGB16) m_mt.subtype = MEDIASUBTYPE_RGB565;
  else if(out_color_mode == YUV)m_mt.subtype = MEDIASUBTYPE_UYVY;
  else {
    //SaveLog.ToLog("\n Color mode is not correct");
    return -1;
  }


  // Note: I don't expect the next few methods to fail ....
  hr = pGrabber->SetMediaType(&m_mt);  // Set the media type we want for the connection.
  hr = pGrabber->SetOneShot(FALSE);  // Disable "one-shot" mode
  hr = pGrabber->SetBufferSamples(FALSE); // Disable sample buffering
  hr = pGrabber->SetCallback(&grabberCallback, 0); // Set our callback. '0' means 'use the SampleCB callback'

#ifdef _YUVtransF_

  hr = CDSUtils::ConnectFilters(pGraph, pUpstream, pUpstreamPinName, m_pYUVtransF, NULL);
  //if(hr < 0) SaveLog.ToLog("\n Can not ConnectFilter : YUV Transform.");
  //SaveGraphFile(pGraph, L"ConnectYUVtransF.GRF");
  hr = CDSUtils::ConnectFilters(pGraph, m_pYUVtransF, L"XForm Out", m_pGrabF, NULL);
  //  hr = CDSUtils::ConnectFilters(pGraph, m_pYUVtransF, pUpstreamPinName, m_pGrabF, NULL);
  //if(hr < 0) SaveLog.ToLog("\n Can not ConnectFilter : Grabber.");
  //SaveGraphFile(pGraph, L"ConnectGrabber.GRF");
#else
  hr = CDSUtils::ConnectFilters(pGraph, pUpstream, pUpstreamPinName, m_pGrabF, NULL);
  if(hr < 0) {
    //SaveLog.ToLog("\n Can not ConnectFilter : Grabber.");
    return hr;
  }
  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"ConnectGrabber.GRF"));
#endif
  // If input is YUV, add YUVtransform filter.
  //SaveLog.ToLog("\n In SetFilter m_sub_type");
  //SaveLog.ToLog(m_sub_type);
  /*
    if(m_sub_type == 0) {

    } else {
    hr = CDSUtils::ConnectFilters(pGraph, pUpstream, NULL, m_pGrabF, NULL);
    if(FAILED(hr)) continue;
    }
  */
  // Find out the exact video format.
  hr = pGrabber->GetConnectedMediaType(&m_mt);
  if (FAILED(hr))
  {
    //SaveLog.ToLog("\nCould not get the video format.");
    return -1;
  }

  VIDEOINFOHEADER *pVih;
  pVih = reinterpret_cast<VIDEOINFOHEADER*>(m_mt.pbFormat);

  if(SetVideoFormat(*pVih, out_color_mode) < 0, sage) return -1;
  CoTaskMemFree(m_mt.pbFormat);

  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"InSetFilter.GRF"));

  //  if(pGrabber) delete pGrabber;
  //  SAFE_RELEASE(pGrabber);

  return hr;
}
HRESULT C_GrabProc::SetAudioFilter(IGraphBuilder* pGraph, IBaseFilter* pUpstream, wchar_t* pUpstreamPinName)
{
  HRESULT hr;
  AudioGrabCallback.m_GrabProc = this;

  EXECUTE_ASSERT(ERROR_SUCCESS == SaveGraphFile(pGraph, L"Before Add filter.GRF"));

  hr = CDSUtils::AddFilter(pGraph, CLSID_SampleGrabber, L"Audio Grabber", &m_pAudioGrabF);
  if(hr < 0) {
    //SaveLog.ToLog("\n Can not Add filter : Audio Grabber.");
    return hr;
  }
  //SaveLog.ToLog("\n Grabber is added ");
  //SaveLog.ToLog((int)&m_pAudioGrabF);
  EXECUTE_ASSERT(ERROR_SUCCESS == SaveGraphFile(pGraph, L"Before connect filter.GRF"));

  CComQIPtr<ISampleGrabber> pGrabber(m_pAudioGrabF);

  // mediatype definition
  /*typedef struct _MediaType {
    GUID     majortype;
    GUID     subtype;
    BOOL     bFixedSizeSamples;
    BOOL     bTemporalCompression;
    ULONG    lSampleSize;
    GUID     formattype;
    IUnknown *pUnk;
    ULONG    cbFormat;
    BYTE     *pbFormat;
    } AM_MEDIA_TYPE;*/

  // Configure the sample grabber
  ZeroMemory(&m_audiomt, sizeof(AM_MEDIA_TYPE));
  m_audiomt.majortype = MEDIATYPE_Audio;
  m_audiomt.subtype = MEDIASUBTYPE_PCM;
  m_audiomt.lSampleSize = 1024 * 2 * 2; //(16 but-> 2byte) // Size of the sample in bytes. For compressed data, the value can be zero
  m_audiomt.bFixedSizeSamples = TRUE;   // 1024 1536
  m_audiomt.bTemporalCompression = FALSE;
  m_audiomt.formattype = FORMAT_WaveFormatEx;

  // Allocate the format block to hold the WAVEFORMATEX structure.
  // waveformat definition
  /*typedef struct
    {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD  nSamplesPerSec;
    DWORD  nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
    } WAVEFORMATEX; *PWAVEFORMATEX;|*/

  WAVEFORMATEX *pwav = new WAVEFORMATEX;
  m_audiomt.pbFormat = (BYTE *)pwav;
  //WAVEFORMATEX *pwav = (WAVEFORMATEX*)m_audiomt.pbFormat;

  //WAVE_FORMAT_PCM   PCM (pulse-code modulated) data in integer format.
  //WAVE_FORMAT_IEEE_FLOAT  PCM data in IEEE floating-point format.
  //WAVE_FORMAT_DRM   DRM-encoded format (for digital-audio content protected by Microsoft Digital Rights Management).
  //WAVE_FORMAT_EXTENSIBLE  Extended WAVEFORMATEX structure (see WAVEFORMATEXTENSIBLE).
  //WAVE_FORMAT_ALAW  A-law-encoded format.
  //WAVE_FORMAT_MULAW   Mu-law-encoded format.
  //WAVE_FORMAT_ADPCM   ADPCM (adaptive differential pulse-code modulated) data.
  //WAVE_FORMAT_MPEG  MPEG-1 data format (stream conforms to ISO 11172-3 Audio specification).
  //WAVE_FORMAT_DOLBY_AC3_SPDIF   AC3 (aka Dolby Digital) over S/PDIF.
  pwav->wFormatTag = WAVE_FORMAT_PCM;

  pwav->nChannels = 2;
  pwav->nSamplesPerSec = 48000;

  // Specifies the number of bits per sample for the format type specified by wFormatTag.
  // If wFormatTag = WAVE_FORMAT_PCM, then wBitsPerSample should be set to either 8 or 16.
  pwav->wBitsPerSample = 16;

  // Specifies the block alignment in bytes. The block alignment is the size of the minimum
  // atomic unit of data for the wFormatTag format type. If wFormatTag = WAVE_FORMAT_PCM,
  // set nBlockAlign to (nChannels*wBitsPerSample)/8, which is the size of a single audio frame.
  pwav->nBlockAlign = (pwav->nChannels * pwav->wBitsPerSample)/8;

  pwav->nAvgBytesPerSec = pwav->nSamplesPerSec * pwav->nBlockAlign;

  // Specifies the size, in bytes, of extra format information appended to the end of the WAVEFORMATEX structure.
  // This information can be used by non-PCM formats to store extra attributes for the wFormatTag.
  pwav->cbSize = 0;

  // Note: I don't expect the next few methods to fail ....
  hr = pGrabber->SetMediaType(&m_audiomt);  // Set the media type we want for the connection.
  hr = pGrabber->SetOneShot(FALSE);  // Disable "one-shot" mode
  hr = pGrabber->SetBufferSamples(FALSE); // Disable sample buffering
  hr = pGrabber->SetCallback(&AudioGrabCallback,0); // Set our callback. '0' means 'use the SampleCB callback'

  hr = CDSUtils::ConnectFilters(pGraph, pUpstream, pUpstreamPinName, m_pAudioGrabF, NULL);
  if(hr < 0) {
    //SaveLog.ToLog("\n Can not ConnectFilter : Audio Grabber.");
    return hr;
  }
  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"ConnectGrabber.GRF"));

  // Find out the exact video format.
  hr = pGrabber->GetConnectedMediaType(&m_audiomt);
  if (FAILED(hr))
  {
    //SaveLog.ToLog("\nCould not get the audio format.");
    return -1;
  }

#ifdef _SAGE_
  //if(m_sageOn == true) {
  if(Out_sail.init(pwav->nAvgBytesPerSec) < 0)
    return -1;

  //} //Out_sail.setSailEnv(NULL, 1);
#endif

  //CoTaskMemFree(m_mt.pbFormat);

  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(pGraph, L"InSetFilter.GRF"));

  return hr;
}

void C_GrabProc::Shutdown()
{
#ifdef _SAGE_
  //if(m_sageOn == true) {
  Out_sail.shutdown();
  //}
  //Out_sail.setSailEnv(NULL, 1);
#endif
}

//-------------------------------------------------------------------------------------------
IBaseFilter* C_GrabProc::GrabF(void) {
  return m_pGrabF;
}

void C_GrabProc::setIP(CString ip)
{
  m_ip = ip;
}

// SetFormat: Sets the image format (height, width, etc).

// Use UYVY only!
/*
  typedef struct tagVIDEOINFOHEADER {

  RECT            rcSource;          // The bit we really want to use
  RECT            rcTarget;          // Where the video should go
  DWORD           dwBitRate;         // Approximate bit data rate
  DWORD           dwBitErrorRate;    // Bit error rate for this stream
  REFERENCE_TIME  AvgTimePerFrame;   // Average time per frame (100ns units)

  BITMAPINFOHEADER bmiHeader;

  } VIDEOINFOHEADER;
*/
//-------------------------------------------------------------------------------------------
HRESULT C_GrabProc::SetVideoFormat(const VIDEOINFOHEADER& vih, int color_mode)
{

  // /*
  // Check if UYVY
  char tmp[125];
  //SaveLog.ToLog("\n In SetFormat ");
  //  SaveLog.ToLog((char*)&(vih.bmiHeader.biCompression), sizeof(DWORD));
  //  SaveLog.ToLog16((char*)&(vih.bmiHeader.biCompression), sizeof(DWORD));

  if (vih.bmiHeader.biCompression == BI_RGB)
  {
    //SaveLog.ToLog("\r\n It is RGB24");
  }

  if (vih.bmiHeader.biCompression == FCC('UYVY'))
  {
    //SaveLog.ToLog("\r\n It's UYVY");
  }

  if (vih.bmiHeader.biCompression == FCC('YUYV'))
  {
    //SaveLog.ToLog("\r\n It's YUYV");
  }

  if (vih.bmiHeader.biCompression == FCC('2YUV'))
  {
    //SaveLog.ToLog("\r\n It's 2YUV");
  }

  if (vih.bmiHeader.biCompression == FCC('YUV2'))
  {
    //SaveLog.ToLog("\r\n It's YUV2");
  }
  /*
    if (vih.bmiHeader.biCompression != FCC('UYVY'))
    {
    //SaveLog.ToLog("\r\n In SetFormat : It's not UYVY");
    return E_INVALIDARG;
    }
    // */
  int BytesPerPixel = vih.bmiHeader.biBitCount / 8;

  // If the target rectangle (rcTarget) is empty, the image width and the stride are both biWidth.
  // Otherwise, image width is given by rcTarget and the stride is given by biWidth.

  if (IsRectEmpty(&vih.rcTarget))
  {
    m_dwWidth = vih.bmiHeader.biWidth;
    m_lStride = m_dwWidth;
  }
  else
  {
    m_dwWidth = vih.rcTarget.right;
    m_lStride = vih.bmiHeader.biWidth;
  }

  m_lStride = (m_lStride * BytesPerPixel + 3) & ~3; // stride for UYVY is rounded to the nearest DWORD

  m_dwHeight = abs(vih.bmiHeader.biHeight);  // biHeight can be < 0, but YUV is always top-down
  m_iBitDepth = vih.bmiHeader.biBitCount;
  m_out_color_mode = color_mode;

  if(m_pbRGB_buff) {
    free(m_pbRGB_buff);
    m_pbRGB_buff = NULL;
  }
  m_bufLen = m_lStride * m_dwHeight;

#ifdef _SAGE_
  //if(m_sageOn == true) {
  //int sail_init(int width, int height, int color_mode,char* output_addr, int output_port);
  //if(Out_sail.init(m_dwWidth, m_dwHeight, m_out_color_mode, "192.168.81.120", 0) < 0)
  char sip[25];
  sprintf(sip, "%s", m_ip.GetBuffer());

  if(Out_sail.init(m_dwWidth, m_dwHeight, m_out_color_mode, sip, 22000) < 0)
    return -1;
  //void setSailEnv(char* execConfigName, int nwID);
  Out_sail.setSailEnv(NULL, 1);
  //}
#endif
  // /*
  //SaveLog.ToLog("\nLog test");

  //SaveLog.ToLog("\n width : ");
  //SaveLog.ToLog(m_dwWidth);
  //SaveLog.ToLog("height : ");
  //SaveLog.ToLog(m_dwHeight);
  // */
  return S_OK;
}

//-------------------------------------------------------------------------------------------
HRESULT C_GrabProc::SetImageBuffer(BYTE *pBuffer)
{
  m_pImg = pBuffer;
  return S_OK;
}

//-------------------------------------------------------------------------------------------
HRESULT C_GrabProc::SetAudioBuffer(BYTE *pBuffer)
{
  /*m_startPoint
    m_avgBytesPerSec

    int m_startPoint;
    int m_avgBytesPerSec;
    int m_bitsPerSample;
    int m_channels;
  */
  // seek first of pointer
  /*if(m_bitsPerSample == 16) {
    short* data = (short*) pBuffer;
    int index = m_startPoint/2;
    m_pAudioSamples = (BYTE *) &data[index];
    // 2 bytes
    }
    else if(m_bitsPerSample == 8){
    char* data = (char*) pBuffer;
    m_pAudioSamples = (BYTE *) &data[m_startPoint];
    // 1 bytes
    }

    m_startPoint += m_fixedSampleSize;
    if(m_startPoint >= m_avgBytesPerSec) {
    m_startPoint =0;
    }*/

  m_pAudioSamples = pBuffer;
  return S_OK;
}


//-------------------------------------------------------------------------------------------
// RGB -> Bmp file
int C_GrabProc::SendToBmp(void) {
  //SaveLog.ToLog("\n SendToBmp ");
  char fileName[125];
  static int index = 0;

  sprintf(fileName, "outBmp_%0d.bmp", ++index);
  SaveToBmp(fileName, m_pImg, m_dwWidth, m_dwHeight, 24);

  return 1;
}

//-------------------------------------------------------------------------------------------
// RGB -> sail
int C_GrabProc::SendToSage(void) {
  //SaveLog.ToLog("\n SendToSage ");
  //void push_data(unsigned char* buf , int buf_len);
  if(!m_pImg) return -1;
  return Out_sail.push_data(m_pImg , m_bufLen);
}

//-------------------------------------------------------------------------------------------
// AUDIOSAMPLES -> sail
int C_GrabProc::SendAudioToSage(int buflen) {
  //SaveLog.ToLog("\n SendToSage ");
  if(!m_pAudioSamples) return -1;
  return Out_sail.push_audiodata(m_pAudioSamples, buflen);
}

//-------------------------------------------------------------------------------------------
// /*
// YUV -> Bmp file or sail.
HRESULT C_GrabProc::ConvertImage()
{
  //SaveLog.ToLog("\n ConvertImage ");
  if (!m_pImg)
  {
    return E_UNEXPECTED;
  }

  if(!m_pbRGB_buff) {
    m_pbRGB_buff = (unsigned char*)malloc(m_dwWidth * m_dwHeight * 3);
  }

  DWORD iRow, iPixel;  // looping variables
  BYTE *pRow = m_pImg; // pointer to the first row in the buffer (don't care about image orientation)
  unsigned char *in_buff, *out_buff;

  UYVY_ARRAY uyvy;

  in_buff = m_pImg;
  out_buff = m_pbRGB_buff;

  for (iRow = 0; iRow < m_dwHeight; iRow++)
  {
    for (iPixel = 0; iPixel < m_dwWidth; iPixel += 2)
    {
      memcpy(&uyvy, in_buff, 4);

      // for saving in a file : BGR
      // for sending to sail : RGB
      *(out_buff) = YUV2Red(uyvy.y0, uyvy.u, uyvy.v);
      *(++out_buff) = YUV2Green(uyvy.y0, uyvy.u, uyvy.v);
      *(++out_buff) = YUV2Blue(uyvy.y0, uyvy.u, uyvy.v);
      out_buff++;

      *(out_buff) = YUV2Red(uyvy.y1, uyvy.u, uyvy.v);
      *(++out_buff) = YUV2Green(uyvy.y1, uyvy.u, uyvy.v);
      *(++out_buff) = YUV2Blue(uyvy.y1, uyvy.u, uyvy.v);
      out_buff++;

      in_buff += 4;

      //      //SaveLog.ToLog("\n iPixel : "); SaveLog.ToLog(iPixel);
    }

    //  SaveLog.ToLog("\n----iRow : "); SaveLog.ToLog(iRow);
  }

#ifdef _SAGE_
  //void push_data(unsigned char* buf , int buf_len);
  Out_sail.push_data(m_pbRGB_buff , 0);
#else
  char fileName[125];
  static int index = 0;
  sprintf(fileName, "outBmp_%03d.bmp", ++index);
  SaveToBmp(fileName, m_pbRGB_buff, m_dwWidth, m_dwHeight, 24);
#endif

  return S_OK;
}

void C_GrabProc::SetAudioOn(bool on)
{
#ifdef _SAGE_
  //void push_data(unsigned char* buf , int buf_len);
  Out_sail.setAudioOn(on);
#endif
}

bool C_GrabProc::GetSageOn()
{
  return m_sageOn;
}

void C_GrabProc::SetAudioCaptureMode(bool on)
{
  if(on == true)
  {
    Out_sail.initAudioCaptureMode();
  }
}
// */\\
