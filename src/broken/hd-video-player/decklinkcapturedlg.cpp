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
// $Id: DecklinkCaptureDlg.cpp,v 1.9 2006/04/11 01:11:07 ivanr Exp $
//
// Desc: DirectShow capture sample
//
// Copyright (c) Blackmagic Design 2005. All rights reserved.
//
// Modified by khchoi (khchoi@netmedia.gist.ac.kr) 2006/08/28
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "DecklinkCapture.h"
#include "DecklinkCaptureDlg.h"
#define _SMART_TREE_
#define _MAIN_
#include "GrabberProc.h"
#include "Utils2.h"

#include <initguid.h> // TODO: move this to a lib
//#include "DecklinkSample_uuids.h"

#undef lstrlenW

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_GRAPHNOTIFY    WM_APP+1    // for Filter Graph event notification

int FrameRate = FR20; // 20fps

//-----------------------------------------------------------------------------
// CAboutDlg
//-----------------------------------------------------------------------------

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
  CAboutDlg();

  // Dialog Data
  enum { IDD = IDD_ABOUTBOX };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

  // Implementation
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// CDecklinkCaptureDlg dialog
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Constructor
//
CDecklinkCaptureDlg::CDecklinkCaptureDlg(CWnd* pParent /*=NULL*/)
: CDialog(CDecklinkCaptureDlg::IDD, pParent)
  , m_pIVW(NULL)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CDecklinkCaptureDlg::~CDecklinkCaptureDlg() {
  grabberCallback.m_GrabProc.FreeFilter();
}

//-----------------------------------------------------------------------------
// DoDataExchange
//
void CDecklinkCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_COMBO_VIDEOFORMATS, m_videoFormatCtrl);
  DDX_Control(pDX, IDC_COMBO_AUDIOFORMATS, m_audioFormatCtrl);

  DDX_Control(pDX, IDC_STATIC_PREVIEW, m_preview);

  DDX_Control(pDX, IDC_EDIT_CAPTUREFILE, m_captureFileCtrl);
  DDX_Control(pDX, IDC_COMBO_COMPRESSION, m_compressionCtrl);
  DDX_Control(pDX, IDC_COMBO_COMPRESSION2, m_framerateCtrl);
  DDX_Control(pDX, IDC_COMBO_VIDEODEVICE, m_videoDeviceCtrl);
  DDX_Control(pDX, IDC_COMBO_AUDIODEVICE, m_audioDeviceCtrl);

  DDX_Control(pDX, IDC_SAGE_ADDR,   m_sageStreamCtrl);
  DDX_Control(pDX, IDC_SAGE_EDIT_IP,  m_sageIPCtrl);

  DDX_Control(pDX, IDC_CHECK_AUDIOMUTE, m_MuteCtrl);
}

BEGIN_MESSAGE_MAP(CDecklinkCaptureDlg, CDialog)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
//}}AFX_MSG_MAP
ON_CBN_SELCHANGE(IDC_COMBO_VIDEOFORMATS, OnCbnSelchangeComboVideoformats)
ON_CBN_SELCHANGE(IDC_COMBO_AUDIOFORMATS, OnCbnSelchangeComboAudioformats)
ON_BN_CLICKED(IDC_CHECK_AUDIOMUTE, OnBnClickedCheckAudiomute)
ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnBnClickedButtonBrowse)
ON_BN_CLICKED(IDC_BUTTON_CAPTURE, OnBnClickedButtonCapture)
ON_BN_CLICKED(IDC_BUTTON_STOP, OnBnClickedButtonStop)
ON_CBN_SELCHANGE(IDC_COMBO_COMPRESSION, OnCbnSelchangeComboCompression)
ON_CBN_SELCHANGE(IDC_COMBO_VIDEODEVICE, OnCbnSelchangeComboVideodevice)
ON_CBN_SELCHANGE(IDC_COMBO_AUDIODEVICE, OnCbnSelchangeComboAudiodevice)
ON_BN_CLICKED(IDC_BUTTON_CODECPROPERTIES, &CDecklinkCaptureDlg::OnBnClickedButtonCodecproperties)
ON_BN_CLICKED(IDC_SAGE_START, &CDecklinkCaptureDlg::OnBnClickedSageStart)
ON_BN_CLICKED(IDC_SAGE_STOP, &CDecklinkCaptureDlg::OnBnClickedSageStop)
ON_CBN_SELCHANGE(IDC_SAGE_ADDR, &CDecklinkCaptureDlg::OnCbnSelchangeSageAddr)
ON_EN_CHANGE(IDC_SAGE_EDIT_IP, &CDecklinkCaptureDlg::OnEnChangeSageEditIp)
ON_BN_CLICKED(IDC_SAGE_REG, &CDecklinkCaptureDlg::OnBnClickedSageReg)
ON_CBN_SELCHANGE(IDC_COMBO_COMPRESSION2, &CDecklinkCaptureDlg::OnCbnSelchangeComboCompression2)
END_MESSAGE_MAP()


//-----------------------------------------------------------------------------
// CDecklinkCaptureDlg message handlers
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// OnInitDialog
// Called before the dialog is displayed, use this message handler to initialise
// our app
BOOL CDecklinkCaptureDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    CString strAboutMenu;
    strAboutMenu.LoadString(IDS_ABOUTBOX);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);     // Set big icon
  SetIcon(m_hIcon, FALSE);    // Set small icon

  // create a basic capture graph and preview the incoming video
  m_pGraph = NULL;
  m_pVideoCapture = NULL;
  m_pAudioCapture = NULL;
  m_pVideoRenderer = NULL;
  m_pSmartT = NULL;

  m_pControl = NULL;

  m_pIVW = NULL;
  m_pMediaEvent = NULL;

  m_ROTRegister = 0;
  m_bAudioMute = FALSE;

  m_compressor = 0;
  m_framerate = 0;
  m_bEnableCompressionCtrl = TRUE;

  m_captureFile = "<Select File>";

  // initialise default video media type
  ZeroMemory(&m_vihDefault, sizeof(m_vihDefault));
  m_vihDefault.AvgTimePerFrame = 333667;
  m_vihDefault.bmiHeader.biWidth = 720;
  m_vihDefault.bmiHeader.biHeight = 486;
  m_vihDefault.bmiHeader.biBitCount = 16;
  m_vihDefault.bmiHeader.biCompression = 'YVYU';

  LoadIpInfo();

  /*m_sageStreamCtrl.InsertString(0, _T("Yorda"));
    m_sageStreamCtrl.InsertString(1, _T("Caesar"));
    m_sageStreamCtrl.InsertString(2, _T("PathFinder"));
    m_sageStreamCtrl.SetCurSel(1);
    m_sageIPCtrl.SetWindowText("67.58.62.21");
  */

  // initialise default audio media type
  ZeroMemory(&m_wfexDefault, sizeof(m_wfexDefault));
  m_wfexDefault.nChannels = 2;  // the only field of interest

  // retrieve last state
  QueryRegistry();

  m_captureFileCtrl.SetWindowText(m_captureFile);

  EnableControls();

  // create a preview graph
  // add the filters that will be used by all the graphs; preview, uncompressed capture, dv capture,
  // mpeg capture and windows media capture
  HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<void**>(&m_pGraph));
  if (SUCCEEDED(hr))
  {
#ifdef _DEBUG
    //    hr = CDSUtils::AddGraphToRot(m_pGraph, &m_ROTRegister);
#endif
    hr = m_pGraph->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&m_pControl));
    if (SUCCEEDED(hr))
    {
      // locate the video capture devices
      hr = PopulateDeviceControl(&CLSID_VideoInputDeviceCategory, &m_videoDeviceCtrl);
      if (SUCCEEDED(hr))
      {
        hr = PopulateDeviceControl(&CLSID_AudioInputDeviceCategory, &m_audioDeviceCtrl);
        if (SUCCEEDED(hr))
        {
          PWSTR pVideoName = (PWSTR)m_videoDeviceCtrl.GetItemData(m_videoDeviceCtrl.SetCurSel(0));
          PWSTR pAudioName = (PWSTR)m_audioDeviceCtrl.GetItemData(m_audioDeviceCtrl.SetCurSel(0));
          if (pVideoName && pAudioName)
          {
            hr = CDSUtils::AddFilter2(m_pGraph, CLSID_VideoInputDeviceCategory, pVideoName, &m_pVideoCapture);
            if (SUCCEEDED(hr))
            {
              hr = CDSUtils::AddFilter2(m_pGraph, CLSID_AudioInputDeviceCategory, pAudioName, &m_pAudioCapture);
              if (SUCCEEDED(hr))
              {
                PopulateVideoControl(); // populate the video format control with the video formats of the currently selected device
                PopulateAudioControl(); // populate the audio format control with the audio formats of the currently selected device
                PopulateCompressionControl();

                // locate video screen renderer for the preview window
                hr = CDSUtils::AddFilter(m_pGraph, CLSID_VideoRendererDefault, L"Video Renderer", &m_pVideoRenderer);
                if (SUCCEEDED(hr))
                {
                  hr = CreatePreviewGraph();
                }
              }
            }
          }
        }
      }
    }
  }

  return TRUE;  // return TRUE  unless you set the focus to a control
}

//-----------------------------------------------------------------------------
// DestroyWindow
// Called when the window is being destroyed, clean up and free all resources.
BOOL CDecklinkCaptureDlg::DestroyWindow()
{
  m_regUtils.Close();
#ifdef _DEBUG
  CDSUtils::RemoveGraphFromRot(m_ROTRegister);
#endif
  DestroyGraph();

  grabberCallback.m_GrabProc.Shutdown();

  SAFE_RELEASE(m_pControl);

  // Hide Video Window and remove owner. This has to be done prior to
  // destroying any window that displays video/still.
  if (m_pIVW)
  {
    m_pIVW->put_Visible(OAFALSE);
    m_pIVW->put_Owner(NULL);
  }

  SAFE_RELEASE(m_pIVW);
  SAFE_RELEASE(m_pMediaEvent);

  grabberCallback.m_GrabProc.FreeFilter();
  SAFE_RELEASE(m_pVideoRenderer);
  SAFE_RELEASE(m_pAudioCapture);
  SAFE_RELEASE(m_pVideoCapture);

  SAFE_RELEASE(m_pGraph);

  // free mediatypes attached to format controls
  int count = m_videoFormatCtrl.GetCount();
  for (int item=0; item<count; ++item)
  {
    DeleteMediaType((AM_MEDIA_TYPE*)m_videoFormatCtrl.GetItemData(item));
  }

  count = m_audioFormatCtrl.GetCount();
  int item;
  for (item=0; item<count; ++item)
  {
    DeleteMediaType((AM_MEDIA_TYPE*)m_audioFormatCtrl.GetItemData(item));
  }

  // release the device names attached to the item's data
  count = m_videoDeviceCtrl.GetCount();
  for (item=0; item<count; ++item)
  {
    PWSTR pName = (PWSTR)m_videoDeviceCtrl.GetItemData(item);
    delete [] pName;
  }

  count = m_audioDeviceCtrl.GetCount();
  for (item=0; item<count; ++item)
  {
    PWSTR pName = (PWSTR)m_audioDeviceCtrl.GetItemData(item);
    delete [] pName;
  }

  return CDialog::DestroyWindow();
}

//-----------------------------------------------------------------------------
// OnSysCommand
//
void CDecklinkCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialog::OnSysCommand(nID, lParam);
  }
}

//-----------------------------------------------------------------------------
// OnPaint
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CDecklinkCaptureDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialog::OnPaint();
  }
}

//-----------------------------------------------------------------------------
// HandleGraphEvent
// At the moment we just read the event, discard it and release memory used to store it.
void CDecklinkCaptureDlg::HandleGraphEvent(void)
{
  LONG lEventCode, lEventParam1, lEventParam2;

  if (!m_pMediaEvent)
  {
    return;
  }

  while (SUCCEEDED(m_pMediaEvent->GetEvent(&lEventCode, reinterpret_cast<LONG_PTR *>(&lEventParam1), reinterpret_cast<LONG_PTR *>(&lEventParam2), 0)))
  {
    // just free memory associated with event
    m_pMediaEvent->FreeEventParams(lEventCode, lEventParam1, lEventParam2);
  }
}

//-----------------------------------------------------------------------------
// WindowProc
// Have to add our own message handling loop to handle events from the preview video
// window and to pass Window events onto it - this is so it redraws itself correctly etc.
LRESULT CDecklinkCaptureDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_GRAPHNOTIFY:
    HandleGraphEvent();
    break;
  }

  // Pass all msgs to video window. vid window exists as child of static
  // picture frame. This ensures video window redraws itself etc.
  if (m_pIVW)
  {
    m_pIVW->NotifyOwnerMessage(reinterpret_cast<LONG_PTR>(m_hWnd) /* from me */, message, wParam, lParam);
  }

  return CDialog::WindowProc(message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// OnQueryDragIcon
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDecklinkCaptureDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

//-----------------------------------------------------------------------------
// CreatePreviewGraph
// Create a graph to preview the input
// NOTE: There are many ways of building graphs, you could opt for the ICaptureGraphBuilder interface which would
// make things are lot simpler, however it doesn't always build the most efficient graphs.
HRESULT CDecklinkCaptureDlg::CreatePreviewGraph()
{
  HRESULT hr = S_OK;

  if (m_pGraph)
  {
    // locate smart-T
    // NOTE: The smart-T appears to hold references to its upstream connections even when its input pin
    // is diconnected.  The smart-T has to be removed from the graph in order to clear these references which
    // is why the filter is enumerated and added every time the preview graph is built and removed whenever
    // it is destroyed.

    if (SUCCEEDED(hr))
    {
      //SaveLog.ToLog("\n For uncompressed in Preview");

      hr = CDSUtils::ConnectFilters(m_pGraph, m_pVideoCapture, NULL, m_pVideoRenderer, NULL);
    }
  }
  else
  {
    hr = E_POINTER;
  }

  if (SUCCEEDED(hr))
  {
    // the video path has been connected, initialise the preview window
    InitialiseVideoPreview();

    // optionally connect the audio path
    if (FALSE == m_bAudioMute)
    {
      // connect the Decklink audio capture pin to the mux
      //hjhur     //hr = CDSUtils::RenderFilter(m_pGraph, m_pAudioCapture, L"Capture");
    }

    // run the graph so that we can preview the input video
    if (m_pControl)
    {
      hr = m_pControl->Run();
    }
    else
    {
      hr = E_POINTER;
    }
  }

  EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(m_pGraph, L"Preview.GRF"));

  return hr;
}


//-----------------------------------------------------------------------------
// CreateCaptureGraph
// Create a graph to capture the input
HRESULT CDecklinkCaptureDlg::CreateCaptureGraph(bool sage)
{
  HRESULT hr = S_OK;

  CButton* pCheck = (CButton*)GetDlgItem(IDC_CHECK_AUDIOMUTE);
  if (pCheck)
  {
    m_bAudioMute = pCheck->GetState() & 0x0003;
    grabberCallback.m_GrabProc.SetAudioOn(!m_bAudioMute);
  }

  if (m_pGraph)
  {
    // locate smart-T
    // NOTE: The smart-T appears to hold references to its upstream connections even when its input pin
    // is diconnected.  The smart-T has to be removed from the graph in order to clear these references which
    // is why the filter is enumerated and added every time the preview graph is built and removed whenever
    // it is destroyed.
#ifdef _SMART_TREE_
    if(m_pSmartT == NULL) {
      //SaveLog.ToLog("\n Create Smart Tree");
      hr = CDSUtils::AddFilter(m_pGraph, CLSID_SmartTee, L"Smart Tee", &m_pSmartT);
    }
#else
    hr = 1;
#endif
    if (SUCCEEDED(hr))
    {
      //SaveLog.ToLog("\n For uncompressed in Capture");
      // uncompressed, mpeg and wm preview
      // create the following:
      //
      //  Decklink Video Capture -> Smart-T -> AVI Decompressor -> Video Renderer
      //  Decklink Audio Capture -> Default Audio Renderer
      //

      // render the preview pin on the smart-T filter
      // first connect the Decklink video capture pin to the smart-T
#ifdef _SMART_TREE_
      hr = CDSUtils::ConnectFilters(m_pGraph, m_pVideoCapture, NULL, m_pSmartT, NULL);
      if(FAILED(hr)) {
        //SaveLog.ToLog("\n Can not connect Smart filter to VideoCature");
        return hr;
      }
#endif
      if (SUCCEEDED(hr))
      {
#ifdef _SMART_TREE_
        int color_mode = m_compressionCtrl.GetItemData(m_compressionCtrl.GetCurSel());
        //SaveLog.ToLog("\n Color mode : ");  SaveLog.ToLog(color_mode);
        hr = grabberCallback.m_GrabProc.SetFilter(m_pGraph, m_pSmartT, L"Preview", color_mode, sage);

        if(FAILED(hr)) {
          //SaveLog.ToLog("\n Can not connect Grabber to Smart filter");
          return hr;
        }
#else
        int color_mode = m_compressionCtrl.GetItemData(m_compressionCtrl.GetCurSel();
                                                       hr = grabberCallback.m_GrabProc.SetFilter(m_pGraph, m_pVideoCapture, L"Capture", color_mode);
#endif
                                                       // now connect the preview pin of the smart-T to the video renderer
                                                       hr = CDSUtils::ConnectFilters(m_pGraph, m_pSmartT, L"Capture", m_pVideoRenderer, NULL);
                                                       if(FAILED(hr)) {
                                                         //SaveLog.ToLog("\n Can not connect VideoRenderer to SmartT");
                                                         return hr;
                                                       }
                                                       }
      }
    }
    else
    {
      hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
      // the video path has been connected, initialise the preview window
      InitialiseVideoPreview();
      // optionally connect the audio path
      if (FALSE == m_bAudioMute)
      {

        int streamID = m_sageStreamCtrl.GetCurSel();
        if (streamID >= 0)
        {
          // connect the Decklink audio capture pin to the mux
          //hr = CDSUtils::RenderFilter(m_pGraph, m_pAudioCapture, L"Capture");
          int audioDevice = m_audioDeviceCtrl.GetCurSel();
          if(audioDevice == 1) {  // from HD cam
            //grabberCallback.m_GrabProc.SetAudioCaptureMode(false);
            hr = grabberCallback.m_GrabProc.SetAudioFilter(m_pGraph, m_pAudioCapture, L"Capture");
          } else {        // capture
            grabberCallback.m_GrabProc.SetAudioCaptureMode(true);
          }
        }
      }

      // run the graph so that we can preview the input video
      if (m_pControl)
      {
        hr = m_pControl->Run();
      }
      else
      {
        hr = E_POINTER;
      }
    }

    EXECUTE_ASSERT(ERROR_SUCCESS ==SaveGraphFile(m_pGraph, L"CreateCaptureGraph.GRF"));
    return hr;
  }


  //-----------------------------------------------------------------------------
  // DestroyGraph
  // Remove all intermediate filters, keep any Decklink and video render filters as
  // these are used by all the graphs.
  HRESULT CDecklinkCaptureDlg::DestroyGraph(bool sage)
  {
    HRESULT hr = S_OK;

    if (m_pGraph && m_pControl)
    {
      m_pControl->Stop();

      // release our outstanding reference on this filter so it can be removed from the graph
#ifdef _SMART_TREE_
      SAFE_RELEASE(m_pSmartT);
#endif

      // retrieve the name of the capture device, don't remove it in this method
      PWSTR pNameVideoCapture = (PWSTR)m_videoDeviceCtrl.GetItemData(m_videoDeviceCtrl.GetCurSel());
      PWSTR pNameAudioCapture = (PWSTR)m_audioDeviceCtrl.GetItemData(m_audioDeviceCtrl.GetCurSel());

      CComPtr<IEnumFilters> pEnum = NULL;
      hr = m_pGraph->EnumFilters(&pEnum);
      if (SUCCEEDED(hr))
      {
        IBaseFilter* pFilter = NULL;
        while (S_OK == pEnum->Next(1, &pFilter, NULL))
        {
          FILTER_INFO filterInfo = {0};
          hr = pFilter->QueryFilterInfo(&filterInfo);
          if (SUCCEEDED(hr))
          {
            SAFE_RELEASE(filterInfo.pGraph);

            if ((NULL == wcsstr(filterInfo.achName, pNameVideoCapture)) && (NULL == wcsstr(filterInfo.achName, pNameAudioCapture)) && (NULL == wcsstr(filterInfo.achName, L"Video Renderer")))
            {
              hr = m_pGraph->RemoveFilter(pFilter);
              if (SUCCEEDED(hr))
              {
                hr = pEnum->Reset();
              }
            }
          }
          SAFE_RELEASE(pFilter);
        }
      }
    }
    else
    {
      hr = E_POINTER;
    }

    return hr;
  }

  //-----------------------------------------------------------------------------
  // InitialiseVideoPreview
  // In short get the video screen renderer to draw into the picture control, which is our preview window
  // the following code sets this up, in addition to adding the HandleGraphEvent and WindowProc methods
  // read the DXSDK docos for more detailed information
  void CDecklinkCaptureDlg::InitialiseVideoPreview(void)
  {
    // modify the preview window
    if (m_pVideoRenderer)
    {
      if (NULL == m_pIVW)
      {
        if (SUCCEEDED(m_pVideoRenderer->QueryInterface(IID_IVideoWindow, reinterpret_cast<void**>(&m_pIVW))))
        {
          // get the window to handle redraws, etc
          // Set msg drain of VideoWindow to point to our dialog window. The dialog's
          // window procedure then handles events from the VideoWindow.
          HRESULT hr = m_pIVW->put_MessageDrain(reinterpret_cast<OAHWND>(m_hWnd));

          if (NULL == m_pMediaEvent)
          {
            // Make graph send WM_GRAPHNOTIFY when it wants our attention see "Learning
            // When an Event Occurs" in the DX9 documentation.
            hr = m_pGraph->QueryInterface(IID_IMediaEventEx, reinterpret_cast<void**>(&m_pMediaEvent));
            if (SUCCEEDED(hr))
            {
              hr = m_pMediaEvent->SetNotifyWindow(reinterpret_cast<OAHWND>(m_hWnd), WM_GRAPHNOTIFY, 0);
            }

            // object created for it.
            RECT rc;
            m_preview.GetClientRect(&rc);
            m_pIVW->SetWindowPosition(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

            // VideoWindow is a child window of the bounding rect
            hr = m_pIVW->put_WindowStyle(WS_CHILD);
            hr = m_pIVW->put_Owner(reinterpret_cast<OAHWND>(m_preview.GetSafeHwnd()));
            hr = m_pIVW->SetWindowForeground(0);
          }
        }
      }
    }
  }

  //-----------------------------------------------------------------------------
  // PopulateDeviceControl
  // Fill device combo box with available devices of the specified category
  HRESULT CDecklinkCaptureDlg::PopulateDeviceControl(const GUID* pCategory, CComboBox* pCtrl)
  {
    HRESULT hr = S_OK;
    if (pCategory && pCtrl)
    {
      // first enumerate the system devices for the specifed class and filter name
      CComPtr<ICreateDevEnum> pSysDevEnum = NULL;
      hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&pSysDevEnum));

      if (SUCCEEDED(hr))
      {
        CComPtr<IEnumMoniker> pEnumCat = NULL;
        hr = pSysDevEnum->CreateClassEnumerator(*pCategory, &pEnumCat, 0);

        if (S_OK == hr)
        {
          IMoniker* pMoniker = NULL;
          bool Loop = true;
          while ((S_OK == pEnumCat->Next(1, &pMoniker, NULL)) && Loop)
          {
            IPropertyBag* pPropBag = NULL;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&pPropBag));

            if (SUCCEEDED(hr))
            {
              VARIANT varName;
              VariantInit(&varName);
              hr = pPropBag->Read(L"FriendlyName", &varName, 0);
              if (SUCCEEDED(hr))
              {
                size_t len = wcslen(varName.bstrVal) + 1;
                PWSTR pName = new WCHAR [len];
                StringCchCopyW(pName, len, varName.bstrVal);
                CW2AEX<> buf(varName.bstrVal);
                pCtrl->SetItemData(pCtrl->AddString(buf), (DWORD)pName);
              }

              VariantClear(&varName);

              // contained within a loop, decrement the reference count
              SAFE_RELEASE(pPropBag);
            }
            SAFE_RELEASE(pMoniker);
          }
        }
      }
    }
    else
    {
      hr = E_POINTER;
    }

    return hr;
  }

  //-----------------------------------------------------------------------------
  // PopulateVideoControl
  // Fill video format combo box with supported video formats using the IAMStreamConfig
  // interface.
  HRESULT CDecklinkCaptureDlg::PopulateVideoControl()
  {
    HRESULT hr = S_OK;

    if (m_pVideoCapture)
    {
      // free mediatypes attached to format controls
      int count = m_videoFormatCtrl.GetCount();
      if (count)
      {
        for (int item=0; item<count; ++item)
        {
          DeleteMediaType((AM_MEDIA_TYPE*)m_videoFormatCtrl.GetItemData(item));
        }
        m_videoFormatCtrl.ResetContent();
      }

      // locate the video capture pin and QI for stream control
      CComPtr<IAMStreamConfig> pISC = NULL;
      hr = CDSUtils::FindPinInterface(m_pVideoCapture, &MEDIATYPE_Video, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
      if (SUCCEEDED(hr))
      {
        // loop through all the capabilities (video formats) and populate the control
        int count, size;
        hr = pISC->GetNumberOfCapabilities(&count, &size);
        if (SUCCEEDED(hr))
        {
          if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == size)
          {
            AM_MEDIA_TYPE* pmt = NULL;
            VIDEO_STREAM_CONFIG_CAPS vscc;
            VIDEOINFOHEADER* pvih = NULL;

            for (int index=0; index<count; ++index)
            {
              hr = pISC->GetStreamCaps(index, &pmt, reinterpret_cast<BYTE*>(&vscc));
              if (SUCCEEDED(hr))
              {
                char buffer[128];
                WORD PixelFormat;
                float FrameRate;

                ZeroMemory(buffer, sizeof(buffer));

                pvih = (VIDEOINFOHEADER*)pmt->pbFormat;

                char* pPixelFormatLUT[] = {"4:2:2", "4:4:4"};
                if (pvih->bmiHeader.biBitCount == 16) PixelFormat = 8;
                else if (pvih->bmiHeader.biBitCount == 20) PixelFormat = 10;
                else PixelFormat = pvih->bmiHeader.biBitCount;

                // provide a useful description of the formats
                if (486 == pvih->bmiHeader.biHeight)
                {
                  if (417083 == pvih->AvgTimePerFrame)
                  {
                    StringCbPrintfA(buffer, sizeof(buffer), "NTSC %d-bit %s (3:2 pulldown removal)", PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                  }
                  else
                  {
                    StringCbPrintfA(buffer, sizeof(buffer), "NTSC %d-bit %s", PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                  }
                }
                else if (576 == pvih->bmiHeader.biHeight)
                {
                  StringCbPrintfA(buffer, sizeof(buffer), "PAL %d-bit %s", PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                }
                else
                {
                  char* pFrameRateFormat[] = {"%.2f", "%.0f"};
                  FrameRate = (float)(long)UNITS / pvih->AvgTimePerFrame;

                  if ((720 == pvih->bmiHeader.biHeight) && (59.94 > FrameRate))
                  {
                    if ((FrameRate - (int)FrameRate) > 0.01)
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD720 %.2fp %d-bit %s (Overcranked 60p)", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                    else
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD720 %.0fp %d-bit %s (Overcranked 60p)", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                  }
                  else if ((720 == pvih->bmiHeader.biHeight) && (59.94 <= FrameRate))
                  {
                    if ((FrameRate - (int)FrameRate) > 0.01)
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD720 %.2fp %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                    else
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD720 %.0fp %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                  }
                  else if ((1080 == pvih->bmiHeader.biHeight) && (50.00 <= FrameRate))
                  {
                    if ((FrameRate - (int)FrameRate) > 0.01)
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD1080 %.2fi %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                    else
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD1080 %.0fi %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                  }
                  else
                  {
                    if ((FrameRate - (int)FrameRate) > 0.01)
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD1080 %.2fPsF %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                    else
                    {
                      StringCbPrintfA(buffer, sizeof(buffer), "HD1080 %.0fPsF %d-bit %s", FrameRate, PixelFormat, pPixelFormatLUT[(30 == PixelFormat)]);
                    }
                  }
                }

                // add the item description to combo box
                int n = m_videoFormatCtrl.AddString(buffer);
                // store media type pointer in item's data section
                //m_videoFormatCtrl.SetItemData(n, (DWORD_PTR)pmt);
                m_videoFormatCtrl.SetItemData(n, (DWORD_PTR)pmt);

                // set default format
                if ((pvih->AvgTimePerFrame == m_vihDefault.AvgTimePerFrame) &&
                    (pvih->bmiHeader.biWidth == m_vihDefault.bmiHeader.biWidth) &&
                    (pvih->bmiHeader.biHeight == m_vihDefault.bmiHeader.biHeight) &&
                    (pvih->bmiHeader.biBitCount == m_vihDefault.bmiHeader.biBitCount))
                {
                  m_videoFormatCtrl.SetCurSel(n);
                  pISC->SetFormat(pmt);
                }
              }
            }
          }
          else
          {
            m_videoFormatCtrl.AddString("ERROR: Unable to retrieve video formats");
          }
        }
      }
    }
    else
    {
      hr = E_POINTER;
    }

    // initial
    m_videoFormatCtrl.SetCurSel(4);
    OnCbnSelchangeComboVideoformats();

    return hr;
  }

  //-----------------------------------------------------------------------------
  // PopulateAudioControl
  // Fill audio format combo box with supported audio formats using the IAMStreamConfig
  // interface.
  HRESULT CDecklinkCaptureDlg::PopulateAudioControl()
  {
    HRESULT hr = S_OK;

    if (m_pAudioCapture)
    {
      // free mediatypes attached to format controls
      int count = m_audioFormatCtrl.GetCount();
      if (count)
      {
        for (int item=0; item<count; ++item)
        {
          DeleteMediaType((AM_MEDIA_TYPE*)m_audioFormatCtrl.GetItemData(item));
        }
        m_audioFormatCtrl.ResetContent();
      }

      // locate the audio capture pin and QI for stream control
      CComPtr<IAMStreamConfig> pISC = NULL;
      hr = CDSUtils::FindPinInterface(m_pAudioCapture, &MEDIATYPE_Audio, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
      if (SUCCEEDED(hr))
      {
        // loop through all the capabilities (audio formats) and populate the control
        int count, size;
        hr = pISC->GetNumberOfCapabilities(&count, &size);
        if (SUCCEEDED(hr))
        {
          if (sizeof(AUDIO_STREAM_CONFIG_CAPS) == size)
          {
            AM_MEDIA_TYPE* pmt = NULL;
            AUDIO_STREAM_CONFIG_CAPS ascc;
            WAVEFORMATEX* pwfex = NULL;

            for (int index=0; index<count; ++index)
            {
              hr = pISC->GetStreamCaps(index, &pmt, reinterpret_cast<BYTE*>(&ascc));
              if (SUCCEEDED(hr))
              {
                char buffer[32];

                ZeroMemory(buffer, sizeof(buffer));

                pwfex = (WAVEFORMATEX*)pmt->pbFormat;

                // provide a useful description of the formats
                if (1 == pwfex->nChannels)
                {
                  StringCbPrintfA(buffer, sizeof(buffer), "%d channel, %2.1fkHz, %d-bit", (int)pwfex->nChannels, (float)pwfex->nSamplesPerSec / 1000, (int)pwfex->wBitsPerSample);
                }
                else
                {
                  StringCbPrintfA(buffer, sizeof(buffer), "%d channels, %2.1fkHz, %d-bit", (int)pwfex->nChannels, (float)pwfex->nSamplesPerSec / 1000, (int)pwfex->wBitsPerSample);
                }

                // add the item description to combo box
                int n = m_audioFormatCtrl.AddString(buffer);
                // store media type pointer in item's data section
                m_audioFormatCtrl.SetItemData(n, (DWORD_PTR)pmt);

                // set default format
                if ((pwfex->wFormatTag == m_wfexDefault.wFormatTag) &&
                    (pwfex->nChannels == m_wfexDefault.nChannels) &&
                    (pwfex->nSamplesPerSec == m_wfexDefault.nSamplesPerSec) &&
                    (pwfex->nAvgBytesPerSec == m_wfexDefault.nAvgBytesPerSec))
                {
                  m_audioFormatCtrl.SetCurSel(n);
                  pISC->SetFormat(pmt);
                }
              }
            }
          }
          else
          {
            m_audioFormatCtrl.AddString("ERROR: Unable to retrieve audio formats");
          }
        }
      }
    }
    else
    {
      hr = E_POINTER;
    }

    // initial
    m_audioFormatCtrl.SetCurSel(1);
    OnCbnSelchangeComboAudioformats();

    return hr;
  }

  //-----------------------------------------------------------------------------
  // PopulateCompressionControl
  // Fill compression control with a selection of video compressors, locate the
  // encoders and add them to the combo box if they exist.
  HRESULT CDecklinkCaptureDlg::PopulateCompressionControl()
  {
    // Pixel format
    int n = m_compressionCtrl.AddString("RGB24");
    m_compressionCtrl.SetItemData(n, (DWORD_PTR)RGB24);

    n = m_compressionCtrl.AddString("RGB16");
    m_compressionCtrl.SetItemData(n, (DWORD_PTR)RGB16);

    n = m_compressionCtrl.AddString("YUV");
    m_compressionCtrl.SetItemData(n, (DWORD_PTR)YUV);

    // Frame rate
    n = m_framerateCtrl.AddString("10fps");
    m_framerateCtrl.SetItemData(n, (DWORD_PTR)FR10);
    n = m_framerateCtrl.AddString("15fps");
    m_framerateCtrl.SetItemData(n, (DWORD_PTR)FR15);
    n = m_framerateCtrl.AddString("20fps");
    m_framerateCtrl.SetItemData(n, (DWORD_PTR)FR20);
    n = m_framerateCtrl.AddString("25fps");
    m_framerateCtrl.SetItemData(n, (DWORD_PTR)FR25);
    n = m_framerateCtrl.AddString("30fps");
    m_framerateCtrl.SetItemData(n, (DWORD_PTR)FR30);

    /*
    // search for the DV encoder, MPEG encoder and WM encoder
    IBaseFilter* pFilter = NULL;
    HRESULT hr = CoCreateInstance(CLSID_DVVideoEnc, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void**>(&pFilter));
    if (SUCCEEDED(hr))
    {
    n = m_compressionCtrl.SetCurSel(m_compressionCtrl.AddString("DV Video Encoder"));
    m_compressionCtrl.SetItemData(n, (DWORD_PTR)ENC_DV);
    SAFE_RELEASE(pFilter);
    }

    hr = CoCreateInstance(CLSID_WMAsfWriter, 0, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void**>(&pFilter));
    if (SUCCEEDED(hr))
    {
    n = m_compressionCtrl.SetCurSel(m_compressionCtrl.AddString("Windows Media Encoder"));
    m_compressionCtrl.SetItemData(n, (DWORD_PTR)ENC_WM);
    SAFE_RELEASE(pFilter);
    }
    */
    // initial
    m_compressor =2;
    m_compressionCtrl.SetCurSel(m_compressor);
    //OnCbnSelchangeComboCompression();

    m_framerate = 2; // 20fps
    m_framerateCtrl.SetCurSel(m_framerate);
    FrameRate = FR20;

    return S_OK;
  }

  //-----------------------------------------------------------------------------
  // OnCbnSelchangeComboVideodevice
  // Rebuild graph with selected capture device
  void CDecklinkCaptureDlg::OnCbnSelchangeComboVideodevice()
  {
    SAFE_RELEASE(m_pVideoCapture);  // release our outstanding reference
    // remove intermediate filters, since the device selection has changed the capture device will also be removed
    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      // rebuild graph with new capture device selection
      PWSTR pName = (PWSTR)m_videoDeviceCtrl.GetItemData(m_videoDeviceCtrl.GetCurSel());
      if (pName)
      {
        hr = CDSUtils::AddFilter2(m_pGraph, CLSID_VideoInputDeviceCategory, pName, &m_pVideoCapture);
        if (SUCCEEDED(hr))
        {
          // as the device has changed get the current operating format so that the control
          // and display this as the current selection
          CComPtr<IAMStreamConfig> pISC = NULL;
          hr = CDSUtils::FindPinInterface(m_pVideoCapture, &MEDIATYPE_Video, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
          if (SUCCEEDED(hr))
          {
            // get the current format of the device to set the current selection of the control
            AM_MEDIA_TYPE* pamt = NULL;
            hr = pISC->GetFormat(&pamt);
            if (SUCCEEDED(hr))
            {
              if (pamt->pbFormat)
              {
                m_vihDefault = *(VIDEOINFOHEADER*)pamt->pbFormat;
              }
              DeleteMediaType(pamt);
            }
          }

          hr = PopulateVideoControl();  // repopulate the control with formats from the new device
          if (SUCCEEDED(hr))
          {
            hr = CreatePreviewGraph();  // rebuild the graph with the new device
          }
        }
      }
      else
      {
        hr = E_POINTER;
      }
    }
  }

  //-----------------------------------------------------------------------------
  // OnCbnSelchangeComboAudiodevice
  // Rebuild graph with selected capture device
  void CDecklinkCaptureDlg::OnCbnSelchangeComboAudiodevice()
  {
    SAFE_RELEASE(m_pAudioCapture);  // release our outstanding reference
    // remove intermediate filters, since the device selection has changed the capture device will also be removed
    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      PWSTR pName = (PWSTR)m_audioDeviceCtrl.GetItemData(m_audioDeviceCtrl.GetCurSel());
      if (pName)
      {
        hr = CDSUtils::AddFilter2(m_pGraph, CLSID_AudioInputDeviceCategory, pName, &m_pAudioCapture);
        if (SUCCEEDED(hr))
        {
          // as the device has changed get the current operating format so that the control
          // and display this as the current selection
          CComPtr<IAMStreamConfig> pISC = NULL;
          hr = CDSUtils::FindPinInterface(m_pAudioCapture, &MEDIATYPE_Audio, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
          if (SUCCEEDED(hr))
          {
            // get the current format of the device to set the current selection of the control
            AM_MEDIA_TYPE* pamt = NULL;
            hr = pISC->GetFormat(&pamt);
            if (SUCCEEDED(hr))
            {
              if (pamt->pbFormat)
              {
                m_wfexDefault = *(WAVEFORMATEX*)pamt->pbFormat;
              }
              DeleteMediaType(pamt);
            }
          }

          hr = PopulateAudioControl();  // repopulate the control with formats from the new device
          if (SUCCEEDED(hr))
          {
            hr = CreatePreviewGraph();  // rebuild the graph with the new device
          }
        }
      }
      else
      {
        hr = E_POINTER;
      }
    }
  }

  //-----------------------------------------------------------------------------
  // OnCbnSelchangeComboVideoformats
  // Rebuild preview graph if format selection changed
  void CDecklinkCaptureDlg::OnCbnSelchangeComboVideoformats()
  {
    HRESULT hr = DestroyGraph();

    if (SUCCEEDED(hr))
    {
      // locate the video capture pin and QI for stream control
      CComPtr<IAMStreamConfig> pISC = NULL;
      hr = CDSUtils::FindPinInterface(m_pVideoCapture, &MEDIATYPE_Video, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
      if (SUCCEEDED(hr))
      {
        // set the new media format
        AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)m_videoFormatCtrl.GetItemData(m_videoFormatCtrl.GetCurSel());
        m_vihDefault = *(VIDEOINFOHEADER*)pmt->pbFormat;
        ASSERT(sizeof(VIDEOINFOHEADER) <= pmt->cbFormat);
        hr = pISC->SetFormat(pmt);
        if (SUCCEEDED(hr))
        {
          // save the new format
          //ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("VideoFormat", reinterpret_cast<const BYTE*>(&m_vihDefault), sizeof(m_vihDefault)));
          m_regUtils.SetBinary("VideoFormat", reinterpret_cast<const BYTE*>(&m_vihDefault), sizeof(m_vihDefault));


          // update compression control, we don't have an HD compression filter so disable compression for HD formats
          if (576 < m_vihDefault.bmiHeader.biHeight)
          {
            m_compressor = 2;
            m_compressionCtrl.SetCurSel(m_compressor);
            // save the new state
            //ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("VideoDecoder", reinterpret_cast<const BYTE*>(&m_compressor), sizeof(m_compressor)));
            m_regUtils.SetBinary("VideoDecoder", reinterpret_cast<const BYTE*>(&m_compressor), sizeof(m_compressor));

            m_bEnableCompressionCtrl = FALSE;
          }
          else
          {
            m_bEnableCompressionCtrl = TRUE;
          }
          EnableControls();

          // rebuild the graph
          hr = CreatePreviewGraph();
        }
      }
    }
  }

  //-----------------------------------------------------------------------------
  // OnCbnSelchangeComboAudioformats
  // Rebuild preview graph if format selection changed
  void CDecklinkCaptureDlg::OnCbnSelchangeComboAudioformats()
  {
    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      // locate the audio capture pin and QI for stream control
      CComPtr<IAMStreamConfig> pISC = NULL;
      hr = CDSUtils::FindPinInterface(m_pAudioCapture, &MEDIATYPE_Audio, PINDIR_OUTPUT, IID_IAMStreamConfig, reinterpret_cast<void**>(&pISC));
      if (SUCCEEDED(hr))
      {
        // set the new media format
        AM_MEDIA_TYPE* pmt = (AM_MEDIA_TYPE*)m_audioFormatCtrl.GetItemData(m_audioFormatCtrl.GetCurSel());
        m_wfexDefault = *(WAVEFORMATEX*)pmt->pbFormat;
        ASSERT(sizeof(WAVEFORMATEX) == pmt->cbFormat);
        hr = pISC->SetFormat(pmt);
        if (SUCCEEDED(hr))
        {
          // save the new format
          //EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("AudioFormat", reinterpret_cast<const BYTE*>(&m_wfexDefault), sizeof(m_wfexDefault)));
          m_regUtils.SetBinary("AudioFormat", reinterpret_cast<const BYTE*>(&m_wfexDefault), sizeof(m_wfexDefault));

          // rebuild the graph
          hr = CreatePreviewGraph();
        }
      }
    }
  }

  //-----------------------------------------------------------------------------
  // OnCbnSelchangeComboCompression
  // Rebuild preview graph if compression selection changed
  void CDecklinkCaptureDlg::OnCbnSelchangeComboCompression()
  {
    /*
      HRESULT hr = DestroyGraph();
      if (SUCCEEDED(hr))
      {
      // save the new state
      m_compressor = m_compressionCtrl.GetCurSel();
      EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("VideoDecoder", reinterpret_cast<const BYTE*>(&m_compressor), sizeof(m_compressor)));
      // rebuild the graph
      hr = CreatePreviewGraph();
      }
    */
  }

  //-----------------------------------------------------------------------------
  // OnBnClickedCheckAudiomute
  // Rebuild the capture graph to reflect the new audio setting
  void CDecklinkCaptureDlg::OnBnClickedCheckAudiomute()
  {
    CButton* pCheck = (CButton*)GetDlgItem(IDC_CHECK_AUDIOMUTE);
    if (pCheck)
    {
      m_bAudioMute = pCheck->GetState() & 0x0003;
      // hjhur
      grabberCallback.m_GrabProc.SetAudioOn(!m_bAudioMute);

      HRESULT hr = DestroyGraph();
      if (SUCCEEDED(hr))
      {
        // save the new state
        //EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("AudioMute", reinterpret_cast<const BYTE*>(&m_bAudioMute), sizeof(m_bAudioMute)));

        m_regUtils.SetBinary("AudioMute", reinterpret_cast<const BYTE*>(&m_bAudioMute), sizeof(m_bAudioMute));

        // rebuild the graph which reflects the new audio setting
        hr = CreatePreviewGraph();
      }
    }
  }

  //-----------------------------------------------------------------------------
  // OnBnClickedButtonBrowse
  // Create a file open dialog to browse for a file location
  void CDecklinkCaptureDlg::OnBnClickedButtonBrowse()
  {
    char BASED_CODE szFilters[] = "Windows Media Files|*.avi;*.asf;*.wmv|All Files (*.*)|*.*||";

    char* pExt[] = {"*.avi", "*.avi", "*.asf;*.wmv"};

    CFileDialog FileDlg(TRUE, "Windows Media Files", pExt[m_compressor], 0, szFilters, this);

    if (FileDlg.DoModal() == IDOK)
    {
      m_captureFile = FileDlg.GetPathName();
      m_captureFileCtrl.SetWindowText(m_captureFile);
    }
  }

  //-----------------------------------------------------------------------------
  // OnBnClickedButtonCapture
  // Create a capture graph a start capture
  void CDecklinkCaptureDlg::OnBnClickedButtonCapture()
  {
    DisableControls();

    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      hr = CreateCaptureGraph();
      if (SUCCEEDED(hr))
      {
        DisableControls();
        return;
      }
    }

    hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      hr = CreatePreviewGraph();
      if (SUCCEEDED(hr))
      {
        EnableControls();
      }
    }

    exit(1);
    return;
  }

  //-----------------------------------------------------------------------------
  // OnBnClickedButtonStop
  // Stop capture and revert to preview
  void CDecklinkCaptureDlg::OnBnClickedButtonStop()
  {
    HRESULT hr = DestroyGraph();
    grabberCallback.m_GrabProc.Shutdown();
    if (SUCCEEDED(hr))
    {
      hr = CreatePreviewGraph();
      if (SUCCEEDED(hr))
      {
        EnableControls();
      }
    }
  }

  //-----------------------------------------------------------------------------
  // EnableControls
  //
  void CDecklinkCaptureDlg::EnableControls(void)
  {
    CWnd* pWnd = GetDlgItem(IDC_COMBO_VIDEODEVICE);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_COMBO_VIDEOFORMATS);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_COMBO_AUDIODEVICE);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_COMBO_AUDIOFORMATS);
    //pWnd->EnableWindow(TRUE);
    pWnd->EnableWindow(FALSE);

    pWnd = GetDlgItem(IDC_CHECK_AUDIOMUTE);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_COMBO_COMPRESSION);
    pWnd->EnableWindow(TRUE);
    //  m_bEnableCompressionCtrl = (576 < m_vihDefault.bmiHeader.biHeight) ? FALSE : TRUE;  // don't have an HDV codec do disable compression control for HD formats
    //  pWnd->EnableWindow(m_bEnableCompressionCtrl);
    pWnd = GetDlgItem(IDC_EDIT_CAPTUREFILE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_BROWSE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_CAPTURE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_STOP);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_SAGE_ADDR);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_SAGE_START);
    pWnd->EnableWindow(TRUE);
    pWnd = GetDlgItem(IDC_SAGE_STOP);
    pWnd->EnableWindow(FALSE);
  }

  //-----------------------------------------------------------------------------
  // DisableControls
  //
  void CDecklinkCaptureDlg::DisableControls(void)
  {
    CWnd* pWnd = GetDlgItem(IDC_COMBO_VIDEODEVICE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_COMBO_VIDEOFORMATS);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_COMBO_AUDIODEVICE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_COMBO_AUDIOFORMATS);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_CHECK_AUDIOMUTE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_COMBO_COMPRESSION);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_EDIT_CAPTUREFILE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_BROWSE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_CAPTURE);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_BUTTON_STOP);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_SAGE_ADDR);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_SAGE_START);
    pWnd->EnableWindow(FALSE);
    pWnd = GetDlgItem(IDC_SAGE_STOP);
    pWnd->EnableWindow(TRUE);
  }

  //-----------------------------------------------------------------------------
  // QueryRegistry
  // retrieve previous media formats from registry
  void CDecklinkCaptureDlg::QueryRegistry(void)
  {
    if (ERROR_SUCCESS == m_regUtils.Open("DecklinkCaptureSample"))
    {
      EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.GetBinary("VideoFormat", reinterpret_cast<LPBYTE>(&m_vihDefault), sizeof(m_vihDefault)));
      EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.GetBinary("AudioFormat", reinterpret_cast<LPBYTE>(&m_wfexDefault), sizeof(m_wfexDefault)));
      EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.GetBinary("AudioMute", reinterpret_cast<LPBYTE>(&m_bAudioMute), sizeof(m_bAudioMute)));
      EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.GetBinary("VideoDecoder", reinterpret_cast<LPBYTE>(&m_compressor), sizeof(m_compressor)));

      WCHAR captureFile[MAX_PATH];
      ZeroMemory(captureFile, sizeof(captureFile));
      //    EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.GetString("CaptureFile", reinterpret_cast<LPBYTE>(captureFile), sizeof(captureFile)));
      m_captureFile = captureFile;
    }
    else
    {
      // create the key and registry values
      if (ERROR_SUCCESS == m_regUtils.Create("DecklinkCaptureSample"))
      {
        EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("VideoFormat", reinterpret_cast<const BYTE*>(&m_vihDefault), sizeof(m_vihDefault)));
        EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("AudioFormat", reinterpret_cast<const BYTE*>(&m_wfexDefault), sizeof(m_wfexDefault)));
        EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("AudioMute", reinterpret_cast<const BYTE*>(&m_bAudioMute), sizeof(m_bAudioMute)));
        EXECUTE_ASSERT(ERROR_SUCCESS == m_regUtils.SetBinary("VideoDecoder", reinterpret_cast<const BYTE*>(&m_compressor), sizeof(m_compressor)));
      }
    }

    // update mute audio check box control
    CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_AUDIOMUTE);
    pButton->SetCheck(m_bAudioMute);
  }


  void CDecklinkCaptureDlg::OnBnClickedButtonCodecproperties()
  {
    // TODO: Add your control notification handler code here
  }

  void CDecklinkCaptureDlg::OnBnClickedSageStart()
  {
    int index = m_sageStreamCtrl.GetCurSel();
    //ChangeMasterIP(index);
    CString iptxt;
    m_sageIPCtrl.GetWindowTextA(iptxt);
    grabberCallback.m_GrabProc.setIP(iptxt);

    // TODO: Add your control notification handler code here
    DisableControls();

    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      hr = CreateCaptureGraph();
      if (SUCCEEDED(hr))
      {
        DisableControls();
        return;
      }
    }

    hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      hr = CreatePreviewGraph();
      if (SUCCEEDED(hr))
      {
        EnableControls();
      }
    }

    exit(1);
    return;
  }

  void CDecklinkCaptureDlg::OnBnClickedSageStop()
  {
    // TODO: Add your control notification handler code here
    HRESULT hr = DestroyGraph();
    if (SUCCEEDED(hr))
    {
      hr = CreatePreviewGraph();
      if (SUCCEEDED(hr))
      {
        EnableControls();
      }
    }
    grabberCallback.m_GrabProc.Shutdown();

    exit(1);
  }

  void CDecklinkCaptureDlg::OnCbnSelchangeSageAddr()
  {
    // TODO: Add your control notification handler code here
    int index = m_sageStreamCtrl.GetCurSel();

    CString iptxt;
    m_sageStreamCtrl.GetWindowTextA(iptxt);

    m_sageIPCtrl.SetWindowText(iptxt);

    /*
      switch (index)
      {
      case 0:
      m_sageIPCtrl.SetWindowText("67.58.62.100");
      break;
      case 1:
      m_sageIPCtrl.SetWindowText("67.58.62.21");
      break;
      case 2:
      m_sageIPCtrl.SetWindowText("67.58.62.23");
      break;
      }
    */
    //ChangeMasterIP(index);
  }

  void CDecklinkCaptureDlg::OnEnChangeSageEditIp()
  {
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
  }

  int CDecklinkCaptureDlg::ChangeMasterIP(int index)
  {
    char fileName[100];
    memset(fileName,' ', 100);
    char *sageDir = getenv("SAGE_DIRECTORY");
    sprintf(fileName, "%s/bin/fsManager.conf", sageDir);

    FILE *fp = fopen(fileName, "w+");

    if (!fp) {
      printf("fail to open fsClient config file\n");
      return -1;
    }

    CString iptxt;
    m_sageIPCtrl.GetWindowTextA(iptxt);
    if(iptxt == "") return -1;
    char sip[25];
    sprintf(sip, "%s", iptxt.GetBuffer());

    switch (index)
    {
    case 0:
      fprintf(fp, "fsManager global %s\nsystemPort 20002\n",sip);
      break;
    case 1:
      fprintf(fp, "fsManager global %s\nsystemPort 20002\n",sip);
      break;
    case 2:
      fprintf(fp, "fsManager global %s\nsystemPort 20002\n",sip);
      break;
    }

    fprintf(fp, "receiverBufSize    100\n");
    fprintf(fp, "rcvNwBufSize 16777216\n");
    fprintf(fp, "sendNwBufSize 1048576\n");
    fprintf(fp, "MTU 8800\n");

    fclose(fp);
    return 1;
  }
  void CDecklinkCaptureDlg::OnBnClickedSageReg()
  {
    char fileName[100];
    memset(fileName,' ', 100);
    char *sageDir = getenv("SAGE_DIRECTORY");
    sprintf(fileName, "%s/bin/ips.sys", sageDir);

    FILE *fp = fopen(fileName, "a+");

    if (!fp) {
      printf("fail to open fsClient config file\n");
      return;
    }

    fseek(fp, 0, SEEK_END);
    CString ip;
    m_sageIPCtrl.GetWindowTextA(ip);

    fprintf(fp, "\n%s",ip.GetBuffer());

    char sip[255];
    memset(sip, 0, 255);
    sprintf(sip, "%s", ip.GetBuffer());

    int index = m_sageStreamCtrl.GetCount();
    m_sageStreamCtrl.InsertString(index, sip);

    fclose(fp);
  }


  int CDecklinkCaptureDlg::LoadIpInfo()
  {
    char fileName[100];
    memset(fileName,' ', 100);
    char *sageDir = getenv("SAGE_DIRECTORY");
    sprintf(fileName, "%s/bin/ips.sys", sageDir);
    //sprintf(fileName, "C:/sources/bijeong/bin/ips.sys");

    FILE *fp = fopen(fileName, "r+");

    if (!fp) {
      printf("fail to open fsClient config file\n");
      return -1;
    }

    char sip[255];
    memset(sip, 0, 255);
    int index =0;

    do {
      fscanf(fp, "%s", &sip);
      if(strlen(sip) > 0) {
        if(index == 0) {
          m_sageIPCtrl.SetWindowText(sip);
        }
        m_sageStreamCtrl.InsertString(index, sip);
        index++;
      }
    } while (!feof(fp));

    fclose(fp);

    m_sageStreamCtrl.SetCurSel(0);
    return 1;
  }
  void CDecklinkCaptureDlg::OnCbnSelchangeComboCompression2()
  {
    // Framerate changed
    DWORD_PTR val = (DWORD_PTR)m_framerateCtrl.GetItemData(m_framerateCtrl.GetCurSel());
    FrameRate = val;
  }
