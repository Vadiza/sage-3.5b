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

//-----------------------------------------------------------------------------
// $Id: DecklinkCaptureDlg.h,v 1.5 2006/04/11 01:11:07 ivanr Exp $
//
// Desc: DirectShow capture sample
//
// Copyright (c) Blackmagic Design 2005. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include "afxwin.h"
#include "Utils.h"

// CDecklinkCaptureDlg dialog
class CDecklinkCaptureDlg : public CDialog
{
  // Construction
public:
  CDecklinkCaptureDlg(CWnd* pParent = NULL);  // standard constructor
  ~CDecklinkCaptureDlg();
  // Dialog Data
  enum { IDD = IDD_DECKLINKCAPTURE_DIALOG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

  afx_msg void OnCbnSelchangeComboVideodevice();
  afx_msg void OnCbnSelchangeComboAudiodevice();
  afx_msg void OnCbnSelchangeComboVideoformats();
  afx_msg void OnCbnSelchangeComboAudioformats();
  afx_msg void OnBnClickedCheckAudiomute();
  afx_msg void OnBnClickedButtonBrowse();
  afx_msg void OnBnClickedButtonCapture();
  afx_msg void OnBnClickedButtonStop();
  afx_msg void OnCbnSelchangeComboCompression();
  afx_msg void OnBnClickedSageStart();
  afx_msg void OnBnClickedSageStop();
  afx_msg void OnCbnSelchangeSageAddr();
  afx_msg void OnEnChangeSageEditIp();
  // Implementation
protected:
  HICON m_hIcon;

  // Generated message map functions
  virtual BOOL OnInitDialog();
  virtual BOOL DestroyWindow();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  DECLARE_MESSAGE_MAP()

  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  void HandleGraphEvent();

private:
  IGraphBuilder* m_pGraph;
  IBaseFilter* m_pVideoCapture;
  IBaseFilter* m_pAudioCapture;
  IBaseFilter* m_pVideoRenderer;
  IBaseFilter* m_pSmartT;

  IMediaControl* m_pControl;

  IVideoWindow* m_pIVW; // for the preview window
  IMediaEventEx* m_pMediaEvent; // for the preview window

  DWORD m_ROTRegister;

  CComboBox m_videoDeviceCtrl;
  CComboBox m_audioDeviceCtrl;
  CComboBox m_videoFormatCtrl;
  CComboBox m_audioFormatCtrl;
  CComboBox m_compressionCtrl;
  CComboBox m_sageStreamCtrl;
  CComboBox m_framerateCtrl;

  CButton m_MuteCtrl;

  CEdit m_sageIPCtrl;

  CStatic m_preview;
  BOOL m_bAudioMute;
  int m_compressor;
  int m_framerate;
  BOOL m_bEnableCompressionCtrl;
  enum { ENC_NONE, ENC_DV, ENC_WM };
  //enum { COLOR_RGB24, COLOR_16 };

  CString m_captureFile;
  CEdit m_captureFileCtrl;

  VIDEOINFOHEADER m_vihDefault;
  WAVEFORMATEX m_wfexDefault;
  CRegUtils m_regUtils;

  HRESULT CreatePreviewGraph();
  HRESULT CreateCaptureGraph(bool sage=false);
  HRESULT CreateUncompressedCaptureGraph();
  HRESULT CreateDVCaptureGraph();
  HRESULT CreateWMCaptureGraph();
  HRESULT ConfigureWMEncoder(IBaseFilter* pASFWriter);
  HRESULT DestroyGraph(bool sage=false);

  void InitialiseVideoPreview(void);

  HRESULT PopulateDeviceControl(const GUID* pCategory, CComboBox* pCtrl);
  HRESULT PopulateVideoControl();
  HRESULT PopulateAudioControl();
  HRESULT PopulateCompressionControl();

  int  ChangeMasterIP(int index);
  void EnableControls(void);
  void DisableControls(void);

  int LoadIpInfo();

  void QueryRegistry(void);
  afx_msg void OnBnClickedButtonCodecproperties();
public:
  afx_msg void OnBnClickedSageReg();
  afx_msg void OnCbnSelchangeComboFramerate();
  afx_msg void OnCbnSelchangeComboCompression2();
};
