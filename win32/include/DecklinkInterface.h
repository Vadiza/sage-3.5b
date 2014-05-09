

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 6.00.0361 */
/* at Wed May 31 15:15:01 2006
 */
/* Compiler settings for .\DecklinkInterface.idl:
   Oicf, W1, Zp8, env=Win32 (32b run)
   protocol : dce , ms_ext, c_ext, robust
   error checks: allocation ref bounds_check enum stub_data
   VC __declspec() decoration level:
   __declspec(uuid()), __declspec(selectany), __declspec(novtable)
   DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __DecklinkInterface_h__
#define __DecklinkInterface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */

#ifndef __IDecklinkKeyer_FWD_DEFINED__
#define __IDecklinkKeyer_FWD_DEFINED__
typedef interface IDecklinkKeyer IDecklinkKeyer;
#endif  /* __IDecklinkKeyer_FWD_DEFINED__ */


#ifndef __IDecklinkRawDeviceControl_FWD_DEFINED__
#define __IDecklinkRawDeviceControl_FWD_DEFINED__
typedef interface IDecklinkRawDeviceControl IDecklinkRawDeviceControl;
#endif  /* __IDecklinkRawDeviceControl_FWD_DEFINED__ */


#ifndef __IDecklinkStatus_FWD_DEFINED__
#define __IDecklinkStatus_FWD_DEFINED__
typedef interface IDecklinkStatus IDecklinkStatus;
#endif  /* __IDecklinkStatus_FWD_DEFINED__ */


#ifndef __IDecklinkCaptureBanner_FWD_DEFINED__
#define __IDecklinkCaptureBanner_FWD_DEFINED__
typedef interface IDecklinkCaptureBanner IDecklinkCaptureBanner;
#endif  /* __IDecklinkCaptureBanner_FWD_DEFINED__ */


#ifndef __IDecklinkIOControl_FWD_DEFINED__
#define __IDecklinkIOControl_FWD_DEFINED__
typedef interface IDecklinkIOControl IDecklinkIOControl;
#endif  /* __IDecklinkIOControl_FWD_DEFINED__ */


#ifndef __IDecklinkMediaSample_FWD_DEFINED__
#define __IDecklinkMediaSample_FWD_DEFINED__
typedef interface IDecklinkMediaSample IDecklinkMediaSample;
#endif  /* __IDecklinkMediaSample_FWD_DEFINED__ */


#ifndef __IDecklinkVideoDesktop_FWD_DEFINED__
#define __IDecklinkVideoDesktop_FWD_DEFINED__
typedef interface IDecklinkVideoDesktop IDecklinkVideoDesktop;
#endif  /* __IDecklinkVideoDesktop_FWD_DEFINED__ */


#ifndef __IDecklinkReferenceClock_FWD_DEFINED__
#define __IDecklinkReferenceClock_FWD_DEFINED__
typedef interface IDecklinkReferenceClock IDecklinkReferenceClock;
#endif  /* __IDecklinkReferenceClock_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter DecklinkVideoCaptureFilter;
#else
typedef struct DecklinkVideoCaptureFilter DecklinkVideoCaptureFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureProperties_FWD_DEFINED__
#define __DecklinkVideoCaptureProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureProperties DecklinkVideoCaptureProperties;
#else
typedef struct DecklinkVideoCaptureProperties DecklinkVideoCaptureProperties;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureProperties_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureProperties2_FWD_DEFINED__
#define __DecklinkVideoCaptureProperties2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureProperties2 DecklinkVideoCaptureProperties2;
#else
typedef struct DecklinkVideoCaptureProperties2 DecklinkVideoCaptureProperties2;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureProperties2_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter DecklinkAudioCaptureFilter;
#else
typedef struct DecklinkAudioCaptureFilter DecklinkAudioCaptureFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureProperties_FWD_DEFINED__
#define __DecklinkAudioCaptureProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureProperties DecklinkAudioCaptureProperties;
#else
typedef struct DecklinkAudioCaptureProperties DecklinkAudioCaptureProperties;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureProperties_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureProperties2_FWD_DEFINED__
#define __DecklinkAudioCaptureProperties2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureProperties2 DecklinkAudioCaptureProperties2;
#else
typedef struct DecklinkAudioCaptureProperties2 DecklinkAudioCaptureProperties2;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureProperties2_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter_FWD_DEFINED__
#define __DecklinkVideoRenderFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter DecklinkVideoRenderFilter;
#else
typedef struct DecklinkVideoRenderFilter DecklinkVideoRenderFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderProperties_FWD_DEFINED__
#define __DecklinkVideoRenderProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderProperties DecklinkVideoRenderProperties;
#else
typedef struct DecklinkVideoRenderProperties DecklinkVideoRenderProperties;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderProperties_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter_FWD_DEFINED__
#define __DecklinkAudioRenderFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter DecklinkAudioRenderFilter;
#else
typedef struct DecklinkAudioRenderFilter DecklinkAudioRenderFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderProperties_FWD_DEFINED__
#define __DecklinkAudioRenderProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderProperties DecklinkAudioRenderProperties;
#else
typedef struct DecklinkAudioRenderProperties DecklinkAudioRenderProperties;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderProperties_FWD_DEFINED__ */


#ifndef __DecklinkCaptureFilter_FWD_DEFINED__
#define __DecklinkCaptureFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkCaptureFilter DecklinkCaptureFilter;
#else
typedef struct DecklinkCaptureFilter DecklinkCaptureFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkCaptureFilter_FWD_DEFINED__ */


#ifndef __DecklinkRenderFilter_FWD_DEFINED__
#define __DecklinkRenderFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkRenderFilter DecklinkRenderFilter;
#else
typedef struct DecklinkRenderFilter DecklinkRenderFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkRenderFilter_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter2_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter2 DecklinkVideoCaptureFilter2;
#else
typedef struct DecklinkVideoCaptureFilter2 DecklinkVideoCaptureFilter2;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter2_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter3_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter3_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter3 DecklinkVideoCaptureFilter3;
#else
typedef struct DecklinkVideoCaptureFilter3 DecklinkVideoCaptureFilter3;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter3_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter4_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter4_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter4 DecklinkVideoCaptureFilter4;
#else
typedef struct DecklinkVideoCaptureFilter4 DecklinkVideoCaptureFilter4;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter4_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter5_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter5_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter5 DecklinkVideoCaptureFilter5;
#else
typedef struct DecklinkVideoCaptureFilter5 DecklinkVideoCaptureFilter5;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter5_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter6_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter6_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter6 DecklinkVideoCaptureFilter6;
#else
typedef struct DecklinkVideoCaptureFilter6 DecklinkVideoCaptureFilter6;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter6_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter7_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter7_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter7 DecklinkVideoCaptureFilter7;
#else
typedef struct DecklinkVideoCaptureFilter7 DecklinkVideoCaptureFilter7;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter7_FWD_DEFINED__ */


#ifndef __DecklinkVideoCaptureFilter8_FWD_DEFINED__
#define __DecklinkVideoCaptureFilter8_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoCaptureFilter8 DecklinkVideoCaptureFilter8;
#else
typedef struct DecklinkVideoCaptureFilter8 DecklinkVideoCaptureFilter8;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoCaptureFilter8_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter2_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter2 DecklinkAudioCaptureFilter2;
#else
typedef struct DecklinkAudioCaptureFilter2 DecklinkAudioCaptureFilter2;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter2_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter3_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter3_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter3 DecklinkAudioCaptureFilter3;
#else
typedef struct DecklinkAudioCaptureFilter3 DecklinkAudioCaptureFilter3;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter3_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter4_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter4_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter4 DecklinkAudioCaptureFilter4;
#else
typedef struct DecklinkAudioCaptureFilter4 DecklinkAudioCaptureFilter4;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter4_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter5_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter5_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter5 DecklinkAudioCaptureFilter5;
#else
typedef struct DecklinkAudioCaptureFilter5 DecklinkAudioCaptureFilter5;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter5_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter6_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter6_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter6 DecklinkAudioCaptureFilter6;
#else
typedef struct DecklinkAudioCaptureFilter6 DecklinkAudioCaptureFilter6;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter6_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter7_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter7_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter7 DecklinkAudioCaptureFilter7;
#else
typedef struct DecklinkAudioCaptureFilter7 DecklinkAudioCaptureFilter7;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter7_FWD_DEFINED__ */


#ifndef __DecklinkAudioCaptureFilter8_FWD_DEFINED__
#define __DecklinkAudioCaptureFilter8_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioCaptureFilter8 DecklinkAudioCaptureFilter8;
#else
typedef struct DecklinkAudioCaptureFilter8 DecklinkAudioCaptureFilter8;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioCaptureFilter8_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter2_FWD_DEFINED__
#define __DecklinkVideoRenderFilter2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter2 DecklinkVideoRenderFilter2;
#else
typedef struct DecklinkVideoRenderFilter2 DecklinkVideoRenderFilter2;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter2_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter3_FWD_DEFINED__
#define __DecklinkVideoRenderFilter3_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter3 DecklinkVideoRenderFilter3;
#else
typedef struct DecklinkVideoRenderFilter3 DecklinkVideoRenderFilter3;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter3_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter4_FWD_DEFINED__
#define __DecklinkVideoRenderFilter4_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter4 DecklinkVideoRenderFilter4;
#else
typedef struct DecklinkVideoRenderFilter4 DecklinkVideoRenderFilter4;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter4_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter5_FWD_DEFINED__
#define __DecklinkVideoRenderFilter5_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter5 DecklinkVideoRenderFilter5;
#else
typedef struct DecklinkVideoRenderFilter5 DecklinkVideoRenderFilter5;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter5_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter6_FWD_DEFINED__
#define __DecklinkVideoRenderFilter6_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter6 DecklinkVideoRenderFilter6;
#else
typedef struct DecklinkVideoRenderFilter6 DecklinkVideoRenderFilter6;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter6_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter7_FWD_DEFINED__
#define __DecklinkVideoRenderFilter7_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter7 DecklinkVideoRenderFilter7;
#else
typedef struct DecklinkVideoRenderFilter7 DecklinkVideoRenderFilter7;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter7_FWD_DEFINED__ */


#ifndef __DecklinkVideoRenderFilter8_FWD_DEFINED__
#define __DecklinkVideoRenderFilter8_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkVideoRenderFilter8 DecklinkVideoRenderFilter8;
#else
typedef struct DecklinkVideoRenderFilter8 DecklinkVideoRenderFilter8;
#endif /* __cplusplus */

#endif  /* __DecklinkVideoRenderFilter8_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter2_FWD_DEFINED__
#define __DecklinkAudioRenderFilter2_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter2 DecklinkAudioRenderFilter2;
#else
typedef struct DecklinkAudioRenderFilter2 DecklinkAudioRenderFilter2;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter2_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter3_FWD_DEFINED__
#define __DecklinkAudioRenderFilter3_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter3 DecklinkAudioRenderFilter3;
#else
typedef struct DecklinkAudioRenderFilter3 DecklinkAudioRenderFilter3;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter3_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter4_FWD_DEFINED__
#define __DecklinkAudioRenderFilter4_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter4 DecklinkAudioRenderFilter4;
#else
typedef struct DecklinkAudioRenderFilter4 DecklinkAudioRenderFilter4;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter4_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter5_FWD_DEFINED__
#define __DecklinkAudioRenderFilter5_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter5 DecklinkAudioRenderFilter5;
#else
typedef struct DecklinkAudioRenderFilter5 DecklinkAudioRenderFilter5;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter5_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter6_FWD_DEFINED__
#define __DecklinkAudioRenderFilter6_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter6 DecklinkAudioRenderFilter6;
#else
typedef struct DecklinkAudioRenderFilter6 DecklinkAudioRenderFilter6;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter6_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter7_FWD_DEFINED__
#define __DecklinkAudioRenderFilter7_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter7 DecklinkAudioRenderFilter7;
#else
typedef struct DecklinkAudioRenderFilter7 DecklinkAudioRenderFilter7;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter7_FWD_DEFINED__ */


#ifndef __DecklinkAudioRenderFilter8_FWD_DEFINED__
#define __DecklinkAudioRenderFilter8_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkAudioRenderFilter8 DecklinkAudioRenderFilter8;
#else
typedef struct DecklinkAudioRenderFilter8 DecklinkAudioRenderFilter8;
#endif /* __cplusplus */

#endif  /* __DecklinkAudioRenderFilter8_FWD_DEFINED__ */


#ifndef __DecklinkUpsampleFilter_FWD_DEFINED__
#define __DecklinkUpsampleFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkUpsampleFilter DecklinkUpsampleFilter;
#else
typedef struct DecklinkUpsampleFilter DecklinkUpsampleFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkUpsampleFilter_FWD_DEFINED__ */


#ifndef __DecklinkEffectsFilter_FWD_DEFINED__
#define __DecklinkEffectsFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkEffectsFilter DecklinkEffectsFilter;
#else
typedef struct DecklinkEffectsFilter DecklinkEffectsFilter;
#endif /* __cplusplus */

#endif  /* __DecklinkEffectsFilter_FWD_DEFINED__ */


#ifndef __DecklinkEffectsProperties_FWD_DEFINED__
#define __DecklinkEffectsProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class DecklinkEffectsProperties DecklinkEffectsProperties;
#else
typedef struct DecklinkEffectsProperties DecklinkEffectsProperties;
#endif /* __cplusplus */

#endif  /* __DecklinkEffectsProperties_FWD_DEFINED__ */


#ifndef __MEDIASUBTYPE_V210_FWD_DEFINED__
#define __MEDIASUBTYPE_V210_FWD_DEFINED__
typedef interface MEDIASUBTYPE_V210 MEDIASUBTYPE_V210;
#endif  /* __MEDIASUBTYPE_V210_FWD_DEFINED__ */


#ifndef __MEDIASUBTYPE_v210a_FWD_DEFINED__
#define __MEDIASUBTYPE_v210a_FWD_DEFINED__
typedef interface MEDIASUBTYPE_v210a MEDIASUBTYPE_v210a;
#endif  /* __MEDIASUBTYPE_v210a_FWD_DEFINED__ */


#ifndef __MEDIASUBTYPE_r210_FWD_DEFINED__
#define __MEDIASUBTYPE_r210_FWD_DEFINED__
typedef interface MEDIASUBTYPE_r210 MEDIASUBTYPE_r210;
#endif  /* __MEDIASUBTYPE_r210_FWD_DEFINED__ */


#ifndef __MEDIASUBTYPE_HDYC_FWD_DEFINED__
#define __MEDIASUBTYPE_HDYC_FWD_DEFINED__
typedef interface MEDIASUBTYPE_HDYC MEDIASUBTYPE_HDYC;
#endif  /* __MEDIASUBTYPE_HDYC_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "strmif.h"

#ifdef __cplusplus
extern "C"{
#endif

  void * __RPC_USER MIDL_user_allocate(size_t);
  void __RPC_USER MIDL_user_free( void * );

#ifndef __IDecklinkKeyer_INTERFACE_DEFINED__
#define __IDecklinkKeyer_INTERFACE_DEFINED__

  /* interface IDecklinkKeyer */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkKeyer;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("9D63ADFC-8D1A-451D-958E-12FA4B1EFD2F")
  IDecklinkKeyer : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE set_AlphaBlendModeOn(
                                                                              /* [in] */ UINT isExternalKey) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE set_AlphaBlendModeOff( void) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE set_AlphaLevel(
                                                                        /* [in] */ UINT alphaLevel) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE set_AlphaAutoBlendSettings(
                                                                                    /* [in] */ UINT rampFrames,
                                                                                    /* [in] */ UINT onFrames,
                                                                                    /* [in] */ UINT offFrames,
                                                                                    /* [in] */ UINT blendProcessRepeats) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE do_AlphaRampOn(
                                                                        /* [in] */ UINT framesDuration) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE do_AlphaRampOff(
                                                                         /* [in] */ UINT framesDuration) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE get_DeviceSupportsKeying( void) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE get_DeviceSupportsExternalKeying( void) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE set_DefaultTimebase(
                                                                             /* [in] */ long Timebase) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkKeyerVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkKeyer * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkKeyer * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkKeyer * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *set_AlphaBlendModeOn )(
                                                                           IDecklinkKeyer * This,
                                                                           /* [in] */ UINT isExternalKey);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *set_AlphaBlendModeOff )(
                                                                            IDecklinkKeyer * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *set_AlphaLevel )(
                                                                     IDecklinkKeyer * This,
                                                                     /* [in] */ UINT alphaLevel);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *set_AlphaAutoBlendSettings )(
                                                                                 IDecklinkKeyer * This,
                                                                                 /* [in] */ UINT rampFrames,
                                                                                 /* [in] */ UINT onFrames,
                                                                                 /* [in] */ UINT offFrames,
                                                                                 /* [in] */ UINT blendProcessRepeats);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *do_AlphaRampOn )(
                                                                     IDecklinkKeyer * This,
                                                                     /* [in] */ UINT framesDuration);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *do_AlphaRampOff )(
                                                                      IDecklinkKeyer * This,
                                                                      /* [in] */ UINT framesDuration);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceSupportsKeying )(
                                                                               IDecklinkKeyer * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceSupportsExternalKeying )(
                                                                                       IDecklinkKeyer * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *set_DefaultTimebase )(
                                                                          IDecklinkKeyer * This,
                                                                          /* [in] */ long Timebase);

    END_INTERFACE
  } IDecklinkKeyerVtbl;

  interface IDecklinkKeyer
  {
    CONST_VTBL struct IDecklinkKeyerVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkKeyer_QueryInterface(This,riid,ppvObject)  \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkKeyer_AddRef(This)             \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkKeyer_Release(This)            \
  (This)->lpVtbl -> Release(This)


#define IDecklinkKeyer_set_AlphaBlendModeOn(This,isExternalKey) \
  (This)->lpVtbl -> set_AlphaBlendModeOn(This,isExternalKey)

#define IDecklinkKeyer_set_AlphaBlendModeOff(This)  \
  (This)->lpVtbl -> set_AlphaBlendModeOff(This)

#define IDecklinkKeyer_set_AlphaLevel(This,alphaLevel)  \
  (This)->lpVtbl -> set_AlphaLevel(This,alphaLevel)

#define IDecklinkKeyer_set_AlphaAutoBlendSettings(This,rampFrames,onFrames,offFrames,blendProcessRepeats) \
  (This)->lpVtbl -> set_AlphaAutoBlendSettings(This,rampFrames,onFrames,offFrames,blendProcessRepeats)

#define IDecklinkKeyer_do_AlphaRampOn(This,framesDuration)  \
  (This)->lpVtbl -> do_AlphaRampOn(This,framesDuration)

#define IDecklinkKeyer_do_AlphaRampOff(This,framesDuration) \
  (This)->lpVtbl -> do_AlphaRampOff(This,framesDuration)

#define IDecklinkKeyer_get_DeviceSupportsKeying(This) \
  (This)->lpVtbl -> get_DeviceSupportsKeying(This)

#define IDecklinkKeyer_get_DeviceSupportsExternalKeying(This) \
  (This)->lpVtbl -> get_DeviceSupportsExternalKeying(This)

#define IDecklinkKeyer_set_DefaultTimebase(This,Timebase) \
  (This)->lpVtbl -> set_DefaultTimebase(This,Timebase)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_set_AlphaBlendModeOn_Proxy(
                                                                                         IDecklinkKeyer * This,
                                                                                         /* [in] */ UINT isExternalKey);


  void __RPC_STUB IDecklinkKeyer_set_AlphaBlendModeOn_Stub(
                                                           IRpcStubBuffer *This,
                                                           IRpcChannelBuffer *_pRpcChannelBuffer,
                                                           PRPC_MESSAGE _pRpcMessage,
                                                           DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_set_AlphaBlendModeOff_Proxy(
                                                                                          IDecklinkKeyer * This);


  void __RPC_STUB IDecklinkKeyer_set_AlphaBlendModeOff_Stub(
                                                            IRpcStubBuffer *This,
                                                            IRpcChannelBuffer *_pRpcChannelBuffer,
                                                            PRPC_MESSAGE _pRpcMessage,
                                                            DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_set_AlphaLevel_Proxy(
                                                                                   IDecklinkKeyer * This,
                                                                                   /* [in] */ UINT alphaLevel);


  void __RPC_STUB IDecklinkKeyer_set_AlphaLevel_Stub(
                                                     IRpcStubBuffer *This,
                                                     IRpcChannelBuffer *_pRpcChannelBuffer,
                                                     PRPC_MESSAGE _pRpcMessage,
                                                     DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_set_AlphaAutoBlendSettings_Proxy(
                                                                                               IDecklinkKeyer * This,
                                                                                               /* [in] */ UINT rampFrames,
                                                                                               /* [in] */ UINT onFrames,
                                                                                               /* [in] */ UINT offFrames,
                                                                                               /* [in] */ UINT blendProcessRepeats);


  void __RPC_STUB IDecklinkKeyer_set_AlphaAutoBlendSettings_Stub(
                                                                 IRpcStubBuffer *This,
                                                                 IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                 PRPC_MESSAGE _pRpcMessage,
                                                                 DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_do_AlphaRampOn_Proxy(
                                                                                   IDecklinkKeyer * This,
                                                                                   /* [in] */ UINT framesDuration);


  void __RPC_STUB IDecklinkKeyer_do_AlphaRampOn_Stub(
                                                     IRpcStubBuffer *This,
                                                     IRpcChannelBuffer *_pRpcChannelBuffer,
                                                     PRPC_MESSAGE _pRpcMessage,
                                                     DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_do_AlphaRampOff_Proxy(
                                                                                    IDecklinkKeyer * This,
                                                                                    /* [in] */ UINT framesDuration);


  void __RPC_STUB IDecklinkKeyer_do_AlphaRampOff_Stub(
                                                      IRpcStubBuffer *This,
                                                      IRpcChannelBuffer *_pRpcChannelBuffer,
                                                      PRPC_MESSAGE _pRpcMessage,
                                                      DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_get_DeviceSupportsKeying_Proxy(
                                                                                             IDecklinkKeyer * This);


  void __RPC_STUB IDecklinkKeyer_get_DeviceSupportsKeying_Stub(
                                                               IRpcStubBuffer *This,
                                                               IRpcChannelBuffer *_pRpcChannelBuffer,
                                                               PRPC_MESSAGE _pRpcMessage,
                                                               DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_get_DeviceSupportsExternalKeying_Proxy(
                                                                                                     IDecklinkKeyer * This);


  void __RPC_STUB IDecklinkKeyer_get_DeviceSupportsExternalKeying_Stub(
                                                                       IRpcStubBuffer *This,
                                                                       IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                       PRPC_MESSAGE _pRpcMessage,
                                                                       DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkKeyer_set_DefaultTimebase_Proxy(
                                                                                        IDecklinkKeyer * This,
                                                                                        /* [in] */ long Timebase);


  void __RPC_STUB IDecklinkKeyer_set_DefaultTimebase_Stub(
                                                          IRpcStubBuffer *This,
                                                          IRpcChannelBuffer *_pRpcChannelBuffer,
                                                          PRPC_MESSAGE _pRpcMessage,
                                                          DWORD *_pdwStubPhase);



#endif  /* __IDecklinkKeyer_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_DecklinkInterface_0386 */
  /* [local] */

  struct DecklinkRawCommandAsync
  {
    byte *command;
    UINT lenCommand;
    long asyncResult;
    byte *response;
    UINT lenResponse;
    long commandComplete;
  } ;


  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0386_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0386_v0_0_s_ifspec;

#ifndef __IDecklinkRawDeviceControl_INTERFACE_DEFINED__
#define __IDecklinkRawDeviceControl_INTERFACE_DEFINED__

  /* interface IDecklinkRawDeviceControl */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkRawDeviceControl;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("72D62DE6-010F-48e6-A251-78CA285BDFE0")
  IDecklinkRawDeviceControl : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendRawCommandSync(
                                                                            /* [in] */ const byte *command,
                                                                            /* [in] */ UINT lenCommand,
                                                                            /* [out][in] */ byte *response,
                                                                            /* [out][in] */ UINT *lenResponse) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SendRawCommandAsync(
                                                                             /* [out][in] */ struct DecklinkRawCommandAsync *args) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkRawDeviceControlVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkRawDeviceControl * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkRawDeviceControl * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkRawDeviceControl * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendRawCommandSync )(
                                                                         IDecklinkRawDeviceControl * This,
                                                                         /* [in] */ const byte *command,
                                                                         /* [in] */ UINT lenCommand,
                                                                         /* [out][in] */ byte *response,
                                                                         /* [out][in] */ UINT *lenResponse);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SendRawCommandAsync )(
                                                                          IDecklinkRawDeviceControl * This,
                                                                          /* [out][in] */ struct DecklinkRawCommandAsync *args);

    END_INTERFACE
  } IDecklinkRawDeviceControlVtbl;

  interface IDecklinkRawDeviceControl
  {
    CONST_VTBL struct IDecklinkRawDeviceControlVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkRawDeviceControl_QueryInterface(This,riid,ppvObject) \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkRawDeviceControl_AddRef(This)  \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkRawDeviceControl_Release(This) \
  (This)->lpVtbl -> Release(This)


#define IDecklinkRawDeviceControl_SendRawCommandSync(This,command,lenCommand,response,lenResponse) \
  (This)->lpVtbl -> SendRawCommandSync(This,command,lenCommand,response,lenResponse)

#define IDecklinkRawDeviceControl_SendRawCommandAsync(This,args)  \
  (This)->lpVtbl -> SendRawCommandAsync(This,args)

#endif /* COBJMACROS */


#endif  /* C style interface */



    /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkRawDeviceControl_SendRawCommandSync_Proxy(
                                                                                                    IDecklinkRawDeviceControl * This,
                                                                                                    /* [in] */ const byte *command,
                                                                                                    /* [in] */ UINT lenCommand,
                                                                                                    /* [out][in] */ byte *response,
                                                                                                    /* [out][in] */ UINT *lenResponse);


  void __RPC_STUB IDecklinkRawDeviceControl_SendRawCommandSync_Stub(
                                                                    IRpcStubBuffer *This,
                                                                    IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                    PRPC_MESSAGE _pRpcMessage,
                                                                    DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkRawDeviceControl_SendRawCommandAsync_Proxy(
                                                                                                   IDecklinkRawDeviceControl * This,
                                                                                                   /* [out][in] */ struct DecklinkRawCommandAsync *args);


  void __RPC_STUB IDecklinkRawDeviceControl_SendRawCommandAsync_Stub(
                                                                     IRpcStubBuffer *This,
                                                                     IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                     PRPC_MESSAGE _pRpcMessage,
                                                                     DWORD *_pdwStubPhase);



#endif  /* __IDecklinkRawDeviceControl_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_DecklinkInterface_0387 */
  /* [local] */

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0387_0001
    { DECKLINK_INPUT_NONE = 0,
      DECKLINK_INPUT_PRESENT  = DECKLINK_INPUT_NONE + 1,
      DECKLINK_INPUT_MAX  = DECKLINK_INPUT_PRESENT + 1
    }   DECKLINK_INPUT;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0387_0002
    { DECKLINK_GENLOCK_NOTSUPPORTED = 0,
      DECKLINK_GENLOCK_NOTCONNECTED = DECKLINK_GENLOCK_NOTSUPPORTED + 1,
      DECKLINK_GENLOCK_LOCKED = DECKLINK_GENLOCK_NOTCONNECTED + 1,
      DECKLINK_GENLOCK_NOTLOCKED  = DECKLINK_GENLOCK_LOCKED + 1,
      DECKLINK_GENLOCK_MAX  = DECKLINK_GENLOCK_NOTLOCKED + 1
    }   DECKLINK_GENLOCK;



  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0387_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0387_v0_0_s_ifspec;

#ifndef __IDecklinkStatus_INTERFACE_DEFINED__
#define __IDecklinkStatus_INTERFACE_DEFINED__

  /* interface IDecklinkStatus */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("15BE165D-BFF5-47f8-8E71-DE4657ABEBE5")
  IDecklinkStatus : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVideoInputStatus(
                                                                             /* [out] */ int *videoStatus,
                                                                             /* [out] */ int *genlockStatus) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RegisterVideoStatusChangeEvent(
                                                                                        /* [in] */ unsigned long event) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkStatusVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkStatus * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkStatus * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkStatus * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetVideoInputStatus )(
                                                                          IDecklinkStatus * This,
                                                                          /* [out] */ int *videoStatus,
                                                                          /* [out] */ int *genlockStatus);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RegisterVideoStatusChangeEvent )(
                                                                                     IDecklinkStatus * This,
                                                                                     /* [in] */ unsigned long event);

    END_INTERFACE
  } IDecklinkStatusVtbl;

  interface IDecklinkStatus
  {
    CONST_VTBL struct IDecklinkStatusVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkStatus_QueryInterface(This,riid,ppvObject) \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkStatus_AddRef(This)            \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkStatus_Release(This)           \
  (This)->lpVtbl -> Release(This)


#define IDecklinkStatus_GetVideoInputStatus(This,videoStatus,genlockStatus) \
  (This)->lpVtbl -> GetVideoInputStatus(This,videoStatus,genlockStatus)

#define IDecklinkStatus_RegisterVideoStatusChangeEvent(This,event)  \
  (This)->lpVtbl -> RegisterVideoStatusChangeEvent(This,event)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkStatus_GetVideoInputStatus_Proxy(
                                                                                         IDecklinkStatus * This,
                                                                                         /* [out] */ int *videoStatus,
                                                                                         /* [out] */ int *genlockStatus);


  void __RPC_STUB IDecklinkStatus_GetVideoInputStatus_Stub(
                                                           IRpcStubBuffer *This,
                                                           IRpcChannelBuffer *_pRpcChannelBuffer,
                                                           PRPC_MESSAGE _pRpcMessage,
                                                           DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkStatus_RegisterVideoStatusChangeEvent_Proxy(
                                                                                                    IDecklinkStatus * This,
                                                                                                    /* [in] */ unsigned long event);


  void __RPC_STUB IDecklinkStatus_RegisterVideoStatusChangeEvent_Stub(
                                                                      IRpcStubBuffer *This,
                                                                      IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                      PRPC_MESSAGE _pRpcMessage,
                                                                      DWORD *_pdwStubPhase);



#endif  /* __IDecklinkStatus_INTERFACE_DEFINED__ */


#ifndef __IDecklinkCaptureBanner_INTERFACE_DEFINED__
#define __IDecklinkCaptureBanner_INTERFACE_DEFINED__

  /* interface IDecklinkCaptureBanner */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkCaptureBanner;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("26D02C91-B25F-40ff-9B39-63B3FABCC518")
  IDecklinkCaptureBanner : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNoInputFrame(
                                                                         /* [out] */ byte **frame) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE BlackVideo(
                                                                    /* [in] */ byte *bmih,
                                                                    /* [in] */ byte *frame) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkCaptureBannerVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkCaptureBanner * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkCaptureBanner * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkCaptureBanner * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNoInputFrame )(
                                                                      IDecklinkCaptureBanner * This,
                                                                      /* [out] */ byte **frame);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *BlackVideo )(
                                                                 IDecklinkCaptureBanner * This,
                                                                 /* [in] */ byte *bmih,
                                                                 /* [in] */ byte *frame);

    END_INTERFACE
  } IDecklinkCaptureBannerVtbl;

  interface IDecklinkCaptureBanner
  {
    CONST_VTBL struct IDecklinkCaptureBannerVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkCaptureBanner_QueryInterface(This,riid,ppvObject)  \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkCaptureBanner_AddRef(This)     \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkCaptureBanner_Release(This)    \
  (This)->lpVtbl -> Release(This)


#define IDecklinkCaptureBanner_GetNoInputFrame(This,frame)  \
  (This)->lpVtbl -> GetNoInputFrame(This,frame)

#define IDecklinkCaptureBanner_BlackVideo(This,bmih,frame)  \
  (This)->lpVtbl -> BlackVideo(This,bmih,frame)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkCaptureBanner_GetNoInputFrame_Proxy(
                                                                                            IDecklinkCaptureBanner * This,
                                                                                            /* [out] */ byte **frame);


  void __RPC_STUB IDecklinkCaptureBanner_GetNoInputFrame_Stub(
                                                              IRpcStubBuffer *This,
                                                              IRpcChannelBuffer *_pRpcChannelBuffer,
                                                              PRPC_MESSAGE _pRpcMessage,
                                                              DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkCaptureBanner_BlackVideo_Proxy(
                                                                                       IDecklinkCaptureBanner * This,
                                                                                       /* [in] */ byte *bmih,
                                                                                       /* [in] */ byte *frame);


  void __RPC_STUB IDecklinkCaptureBanner_BlackVideo_Stub(
                                                         IRpcStubBuffer *This,
                                                         IRpcChannelBuffer *_pRpcChannelBuffer,
                                                         PRPC_MESSAGE _pRpcMessage,
                                                         DWORD *_pdwStubPhase);



#endif  /* __IDecklinkCaptureBanner_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_DecklinkInterface_0389 */
  /* [local] */

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0001
    { DECKLINK_BLACKINCAPTURE_NONE  = 0,
      DECKLINK_BLACKINCAPTURE_DIGITAL = DECKLINK_BLACKINCAPTURE_NONE + 1,
      DECKLINK_BLACKINCAPTURE_ANALOGUE  = DECKLINK_BLACKINCAPTURE_DIGITAL + 1,
      DECKLINK_BLACKINCAPTURE_MAX = DECKLINK_BLACKINCAPTURE_ANALOGUE + 1
    }   DECKLINK_BLACKINCAPTURE;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0002
    { DECKLINK_HDDOWNCONVERSION_OFF = 0,
      DECKLINK_HDDOWNCONVERSION_LB16X9  = DECKLINK_HDDOWNCONVERSION_OFF + 1,
      DECKLINK_HDDOWNCONVERSION_ANA = DECKLINK_HDDOWNCONVERSION_LB16X9 + 1,
      DECKLINK_HDDOWNCONVERSION_CENTER  = DECKLINK_HDDOWNCONVERSION_ANA + 1,
      DECKLINK_HDDOWNCONVERSION_MAX = DECKLINK_HDDOWNCONVERSION_CENTER + 1
    }   DECKLINK_HDDOWNCONVERSION;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0003
    { DECKLINK_AUDIOINPUTSOURCE_SDI = 0,
      DECKLINK_AUDIOINPUTSOURCE_AESEBU  = DECKLINK_AUDIOINPUTSOURCE_SDI + 1,
      DECKLINK_AUDIOINPUTSOURCE_ANALOGUE  = DECKLINK_AUDIOINPUTSOURCE_AESEBU + 1,
      DECKLINK_AUDIOINPUTSOURCE_MAX = DECKLINK_AUDIOINPUTSOURCE_ANALOGUE + 1
    }   DECKLINK_AUDIOINPUTSOURCE;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0004
    { DECKLINK_IOFEATURES_SUPPORTSINTERNALKEY = 1 << 4,
      DECKLINK_IOFEATURES_SUPPORTSEXTERNALKEY = 1 << 5,
      DECKLINK_IOFEATURES_HASCOMPONENTVIDEOOUTPUT = 1 << 6,
      DECKLINK_IOFEATURES_HASCOMPOSITEVIDEOOUTPUT = 1 << 7,
      DECKLINK_IOFEATURES_HASDIGITALVIDEOOUTPUT = 1 << 8,
      DECKLINK_IOFEATURES_HASDVIVIDEOOUTPUT = 1 << 9,
      DECKLINK_IOFEATURES_HASCOMPONENTVIDEOINPUT  = 1 << 10,
      DECKLINK_IOFEATURES_HASCOMPOSITEVIDEOINPUT  = 1 << 11,
      DECKLINK_IOFEATURES_HASDIGITALVIDEOINPUT  = 1 << 12,
      DECKLINK_IOFEATURES_HASDUALLINKOUTPUT = 1 << 13,
      DECKLINK_IOFEATURES_HASDUALLINKINPUT  = 1 << 14,
      DECKLINK_IOFEATURES_SUPPORTSHD  = 1 << 15,
      DECKLINK_IOFEATURES_SUPPORTS2KOUTPUT  = 1 << 16,
      DECKLINK_IOFEATURES_SUPPORTSHDDOWNCONVERSION  = 1 << 17,
      DECKLINK_IOFEATURES_HASAESAUDIOINPUT  = 1 << 18,
      DECKLINK_IOFEATURES_HASANALOGUEAUDIOINPUT = 1 << 19,
      DECKLINK_IOFEATURES_HASSVIDEOINPUT  = 1 << 20,
      DECKLINK_IOFEATURES_HASSVIDEOOUTPUT = 1 << 21,
      DECKLINK_IOFEATURES_SUPPORTSMULTICAMERAINPUT  = 1 << 22,
      DECKLINK_IOFEATURES_MAX = DECKLINK_IOFEATURES_SUPPORTSMULTICAMERAINPUT + 1
    }   DECKLINK_IOFEATURES;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0005
    { DECKLINK_TIMECODESOURCE_VITC  = 0,
      DECKLINK_TIMECODESOURCE_HANC  = DECKLINK_TIMECODESOURCE_VITC + 1,
      DECKLINK_TIMECODESOURCE_MAX = DECKLINK_TIMECODESOURCE_HANC + 1
    }   DECKLINK_TIMECODESOURCE;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0006
    { DECKLINK_VIDEOINPUT_SDI = 0,
      DECKLINK_VIDEOINPUT_COMPONENT = DECKLINK_VIDEOINPUT_SDI + 1,
      DECKLINK_VIDEOINPUT_COMPOSITE = DECKLINK_VIDEOINPUT_COMPONENT + 1,
      DECKLINK_VIDEOINPUT_SVIDEO  = DECKLINK_VIDEOINPUT_COMPOSITE + 1,
      DECKLINK_VIDEOINPUT_MAX = DECKLINK_VIDEOINPUT_SVIDEO + 1
    }   DECKLINK_VIDEOINPUT;

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0389_0007
    { DECKLINK_VIDEOOUTPUT_COMPONENT  = 0,
      DECKLINK_VIDEOOUTPUT_COMPOSITE  = DECKLINK_VIDEOOUTPUT_COMPONENT + 1,
      DECKLINK_VIDEOOUTPUT_SVIDEO = DECKLINK_VIDEOOUTPUT_COMPOSITE + 1,
      DECKLINK_VIDEOOUTPUT_MAX  = DECKLINK_VIDEOOUTPUT_SVIDEO + 1
    }   DECKLINK_VIDEOOUTPUT;



  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0389_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0389_v0_0_s_ifspec;

#ifndef __IDecklinkIOControl_INTERFACE_DEFINED__
#define __IDecklinkIOControl_INTERFACE_DEFINED__

  /* interface IDecklinkIOControl */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkIOControl;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("60F58A81-A387-4922-AAAC-998BD9FBE1AA")
  IDecklinkIOControl : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIOFeatures(
                                                                       /* [out] */ unsigned long *features) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAnalogueOutput(
                                                                           /* [in] */ BOOL isComponent,
                                                                           /* [in] */ BOOL setupIs75) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVideoInput(
                                                                       /* [in] */ BOOL inputIsDigital,
                                                                       /* [in] */ BOOL isComponent,
                                                                       /* [in] */ BOOL setupIs75) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDualLinkOutput(
                                                                           /* [in] */ BOOL enableDualLinkOutput) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSingleFieldOutputForSynchronousFrames(
                                                                                                  /* [in] */ BOOL singleFieldOutput) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetHDTVPulldownOnOutput(
                                                                                 /* [in] */ BOOL enableHDTV32PulldownOnOutput) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetBlackToDeckInCapture(
                                                                                 /* [in] */ unsigned long blackToDeckSetting) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAFrameReference(
                                                                            /* [in] */ unsigned long aFrameReference) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCaptureVANCLines(
                                                                             /* [in] */ unsigned long vancLine1,
                                                                             /* [in] */ unsigned long vancLine2,
                                                                             /* [in] */ unsigned long vancLine3) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVideoOutputDownconversionMode(
                                                                                          /* [in] */ unsigned long downconversionMode) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAudioInputSource(
                                                                             /* [in] */ unsigned long audioInputSource) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetGenlockTiming(
                                                                          /* [in] */ int timingOffset) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVideoOutputDownconversionMode2(
                                                                                           /* [in] */ unsigned long downconversionMode,
                                                                                           /* [in] */ unsigned long downconvertToAnalogOutput) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCaptureTimecodeSource(
                                                                                  /* [in] */ unsigned long timecodeSource) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVideoInput2(
                                                                        /* [in] */ unsigned long videoSource,
                                                                        /* [in] */ BOOL setupIs75,
                                                                        /* [in] */ BOOL componentLevelsSMPTE) = 0;

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAnalogueOutput2(
                                                                            /* [in] */ unsigned long videoOutput,
                                                                            /* [in] */ BOOL setupIs75,
                                                                            /* [in] */ BOOL componentLevelsSMPTE) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkIOControlVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkIOControl * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkIOControl * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkIOControl * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetIOFeatures )(
                                                                    IDecklinkIOControl * This,
                                                                    /* [out] */ unsigned long *features);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAnalogueOutput )(
                                                                        IDecklinkIOControl * This,
                                                                        /* [in] */ BOOL isComponent,
                                                                        /* [in] */ BOOL setupIs75);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVideoInput )(
                                                                    IDecklinkIOControl * This,
                                                                    /* [in] */ BOOL inputIsDigital,
                                                                    /* [in] */ BOOL isComponent,
                                                                    /* [in] */ BOOL setupIs75);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetDualLinkOutput )(
                                                                        IDecklinkIOControl * This,
                                                                        /* [in] */ BOOL enableDualLinkOutput);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetSingleFieldOutputForSynchronousFrames )(
                                                                                               IDecklinkIOControl * This,
                                                                                               /* [in] */ BOOL singleFieldOutput);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetHDTVPulldownOnOutput )(
                                                                              IDecklinkIOControl * This,
                                                                              /* [in] */ BOOL enableHDTV32PulldownOnOutput);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetBlackToDeckInCapture )(
                                                                              IDecklinkIOControl * This,
                                                                              /* [in] */ unsigned long blackToDeckSetting);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAFrameReference )(
                                                                         IDecklinkIOControl * This,
                                                                         /* [in] */ unsigned long aFrameReference);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetCaptureVANCLines )(
                                                                          IDecklinkIOControl * This,
                                                                          /* [in] */ unsigned long vancLine1,
                                                                          /* [in] */ unsigned long vancLine2,
                                                                          /* [in] */ unsigned long vancLine3);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVideoOutputDownconversionMode )(
                                                                                       IDecklinkIOControl * This,
                                                                                       /* [in] */ unsigned long downconversionMode);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAudioInputSource )(
                                                                          IDecklinkIOControl * This,
                                                                          /* [in] */ unsigned long audioInputSource);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetGenlockTiming )(
                                                                       IDecklinkIOControl * This,
                                                                       /* [in] */ int timingOffset);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVideoOutputDownconversionMode2 )(
                                                                                        IDecklinkIOControl * This,
                                                                                        /* [in] */ unsigned long downconversionMode,
                                                                                        /* [in] */ unsigned long downconvertToAnalogOutput);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetCaptureTimecodeSource )(
                                                                               IDecklinkIOControl * This,
                                                                               /* [in] */ unsigned long timecodeSource);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVideoInput2 )(
                                                                     IDecklinkIOControl * This,
                                                                     /* [in] */ unsigned long videoSource,
                                                                     /* [in] */ BOOL setupIs75,
                                                                     /* [in] */ BOOL componentLevelsSMPTE);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAnalogueOutput2 )(
                                                                         IDecklinkIOControl * This,
                                                                         /* [in] */ unsigned long videoOutput,
                                                                         /* [in] */ BOOL setupIs75,
                                                                         /* [in] */ BOOL componentLevelsSMPTE);

    END_INTERFACE
  } IDecklinkIOControlVtbl;

  interface IDecklinkIOControl
  {
    CONST_VTBL struct IDecklinkIOControlVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkIOControl_QueryInterface(This,riid,ppvObject)  \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkIOControl_AddRef(This)         \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkIOControl_Release(This)        \
  (This)->lpVtbl -> Release(This)


#define IDecklinkIOControl_GetIOFeatures(This,features) \
  (This)->lpVtbl -> GetIOFeatures(This,features)

#define IDecklinkIOControl_SetAnalogueOutput(This,isComponent,setupIs75) \
  (This)->lpVtbl -> SetAnalogueOutput(This,isComponent,setupIs75)

#define IDecklinkIOControl_SetVideoInput(This,inputIsDigital,isComponent,setupIs75) \
  (This)->lpVtbl -> SetVideoInput(This,inputIsDigital,isComponent,setupIs75)

#define IDecklinkIOControl_SetDualLinkOutput(This,enableDualLinkOutput) \
  (This)->lpVtbl -> SetDualLinkOutput(This,enableDualLinkOutput)

#define IDecklinkIOControl_SetSingleFieldOutputForSynchronousFrames(This,singleFieldOutput) \
  (This)->lpVtbl -> SetSingleFieldOutputForSynchronousFrames(This,singleFieldOutput)

#define IDecklinkIOControl_SetHDTVPulldownOnOutput(This,enableHDTV32PulldownOnOutput) \
  (This)->lpVtbl -> SetHDTVPulldownOnOutput(This,enableHDTV32PulldownOnOutput)

#define IDecklinkIOControl_SetBlackToDeckInCapture(This,blackToDeckSetting) \
  (This)->lpVtbl -> SetBlackToDeckInCapture(This,blackToDeckSetting)

#define IDecklinkIOControl_SetAFrameReference(This,aFrameReference) \
  (This)->lpVtbl -> SetAFrameReference(This,aFrameReference)

#define IDecklinkIOControl_SetCaptureVANCLines(This,vancLine1,vancLine2,vancLine3) \
  (This)->lpVtbl -> SetCaptureVANCLines(This,vancLine1,vancLine2,vancLine3)

#define IDecklinkIOControl_SetVideoOutputDownconversionMode(This,downconversionMode) \
  (This)->lpVtbl -> SetVideoOutputDownconversionMode(This,downconversionMode)

#define IDecklinkIOControl_SetAudioInputSource(This,audioInputSource) \
  (This)->lpVtbl -> SetAudioInputSource(This,audioInputSource)

#define IDecklinkIOControl_SetGenlockTiming(This,timingOffset)  \
  (This)->lpVtbl -> SetGenlockTiming(This,timingOffset)

#define IDecklinkIOControl_SetVideoOutputDownconversionMode2(This,downconversionMode,downconvertToAnalogOutput) \
  (This)->lpVtbl -> SetVideoOutputDownconversionMode2(This,downconversionMode,downconvertToAnalogOutput)

#define IDecklinkIOControl_SetCaptureTimecodeSource(This,timecodeSource) \
  (This)->lpVtbl -> SetCaptureTimecodeSource(This,timecodeSource)

#define IDecklinkIOControl_SetVideoInput2(This,videoSource,setupIs75,componentLevelsSMPTE) \
  (This)->lpVtbl -> SetVideoInput2(This,videoSource,setupIs75,componentLevelsSMPTE)

#define IDecklinkIOControl_SetAnalogueOutput2(This,videoOutput,setupIs75,componentLevelsSMPTE) \
  (This)->lpVtbl -> SetAnalogueOutput2(This,videoOutput,setupIs75,componentLevelsSMPTE)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_GetIOFeatures_Proxy(
                                                                                      IDecklinkIOControl * This,
                                                                                      /* [out] */ unsigned long *features);


  void __RPC_STUB IDecklinkIOControl_GetIOFeatures_Stub(
                                                        IRpcStubBuffer *This,
                                                        IRpcChannelBuffer *_pRpcChannelBuffer,
                                                        PRPC_MESSAGE _pRpcMessage,
                                                        DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetAnalogueOutput_Proxy(
                                                                                          IDecklinkIOControl * This,
                                                                                          /* [in] */ BOOL isComponent,
                                                                                          /* [in] */ BOOL setupIs75);


  void __RPC_STUB IDecklinkIOControl_SetAnalogueOutput_Stub(
                                                            IRpcStubBuffer *This,
                                                            IRpcChannelBuffer *_pRpcChannelBuffer,
                                                            PRPC_MESSAGE _pRpcMessage,
                                                            DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetVideoInput_Proxy(
                                                                                      IDecklinkIOControl * This,
                                                                                      /* [in] */ BOOL inputIsDigital,
                                                                                      /* [in] */ BOOL isComponent,
                                                                                      /* [in] */ BOOL setupIs75);


  void __RPC_STUB IDecklinkIOControl_SetVideoInput_Stub(
                                                        IRpcStubBuffer *This,
                                                        IRpcChannelBuffer *_pRpcChannelBuffer,
                                                        PRPC_MESSAGE _pRpcMessage,
                                                        DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetDualLinkOutput_Proxy(
                                                                                          IDecklinkIOControl * This,
                                                                                          /* [in] */ BOOL enableDualLinkOutput);


  void __RPC_STUB IDecklinkIOControl_SetDualLinkOutput_Stub(
                                                            IRpcStubBuffer *This,
                                                            IRpcChannelBuffer *_pRpcChannelBuffer,
                                                            PRPC_MESSAGE _pRpcMessage,
                                                            DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetSingleFieldOutputForSynchronousFrames_Proxy(
                                                                                                                 IDecklinkIOControl * This,
                                                                                                                 /* [in] */ BOOL singleFieldOutput);


  void __RPC_STUB IDecklinkIOControl_SetSingleFieldOutputForSynchronousFrames_Stub(
                                                                                   IRpcStubBuffer *This,
                                                                                   IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                                   PRPC_MESSAGE _pRpcMessage,
                                                                                   DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetHDTVPulldownOnOutput_Proxy(
                                                                                                IDecklinkIOControl * This,
                                                                                                /* [in] */ BOOL enableHDTV32PulldownOnOutput);


  void __RPC_STUB IDecklinkIOControl_SetHDTVPulldownOnOutput_Stub(
                                                                  IRpcStubBuffer *This,
                                                                  IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                  PRPC_MESSAGE _pRpcMessage,
                                                                  DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetBlackToDeckInCapture_Proxy(
                                                                                                IDecklinkIOControl * This,
                                                                                                /* [in] */ unsigned long blackToDeckSetting);


  void __RPC_STUB IDecklinkIOControl_SetBlackToDeckInCapture_Stub(
                                                                  IRpcStubBuffer *This,
                                                                  IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                  PRPC_MESSAGE _pRpcMessage,
                                                                  DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetAFrameReference_Proxy(
                                                                                           IDecklinkIOControl * This,
                                                                                           /* [in] */ unsigned long aFrameReference);


  void __RPC_STUB IDecklinkIOControl_SetAFrameReference_Stub(
                                                             IRpcStubBuffer *This,
                                                             IRpcChannelBuffer *_pRpcChannelBuffer,
                                                             PRPC_MESSAGE _pRpcMessage,
                                                             DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetCaptureVANCLines_Proxy(
                                                                                            IDecklinkIOControl * This,
                                                                                            /* [in] */ unsigned long vancLine1,
                                                                                            /* [in] */ unsigned long vancLine2,
                                                                                            /* [in] */ unsigned long vancLine3);


  void __RPC_STUB IDecklinkIOControl_SetCaptureVANCLines_Stub(
                                                              IRpcStubBuffer *This,
                                                              IRpcChannelBuffer *_pRpcChannelBuffer,
                                                              PRPC_MESSAGE _pRpcMessage,
                                                              DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetVideoOutputDownconversionMode_Proxy(
                                                                                                         IDecklinkIOControl * This,
                                                                                                         /* [in] */ unsigned long downconversionMode);


  void __RPC_STUB IDecklinkIOControl_SetVideoOutputDownconversionMode_Stub(
                                                                           IRpcStubBuffer *This,
                                                                           IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                           PRPC_MESSAGE _pRpcMessage,
                                                                           DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetAudioInputSource_Proxy(
                                                                                            IDecklinkIOControl * This,
                                                                                            /* [in] */ unsigned long audioInputSource);


  void __RPC_STUB IDecklinkIOControl_SetAudioInputSource_Stub(
                                                              IRpcStubBuffer *This,
                                                              IRpcChannelBuffer *_pRpcChannelBuffer,
                                                              PRPC_MESSAGE _pRpcMessage,
                                                              DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetGenlockTiming_Proxy(
                                                                                         IDecklinkIOControl * This,
                                                                                         /* [in] */ int timingOffset);


  void __RPC_STUB IDecklinkIOControl_SetGenlockTiming_Stub(
                                                           IRpcStubBuffer *This,
                                                           IRpcChannelBuffer *_pRpcChannelBuffer,
                                                           PRPC_MESSAGE _pRpcMessage,
                                                           DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetVideoOutputDownconversionMode2_Proxy(
                                                                                                          IDecklinkIOControl * This,
                                                                                                          /* [in] */ unsigned long downconversionMode,
                                                                                                          /* [in] */ unsigned long downconvertToAnalogOutput);


  void __RPC_STUB IDecklinkIOControl_SetVideoOutputDownconversionMode2_Stub(
                                                                            IRpcStubBuffer *This,
                                                                            IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                            PRPC_MESSAGE _pRpcMessage,
                                                                            DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetCaptureTimecodeSource_Proxy(
                                                                                                 IDecklinkIOControl * This,
                                                                                                 /* [in] */ unsigned long timecodeSource);


  void __RPC_STUB IDecklinkIOControl_SetCaptureTimecodeSource_Stub(
                                                                   IRpcStubBuffer *This,
                                                                   IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                   PRPC_MESSAGE _pRpcMessage,
                                                                   DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetVideoInput2_Proxy(
                                                                                       IDecklinkIOControl * This,
                                                                                       /* [in] */ unsigned long videoSource,
                                                                                       /* [in] */ BOOL setupIs75,
                                                                                       /* [in] */ BOOL componentLevelsSMPTE);


  void __RPC_STUB IDecklinkIOControl_SetVideoInput2_Stub(
                                                         IRpcStubBuffer *This,
                                                         IRpcChannelBuffer *_pRpcChannelBuffer,
                                                         PRPC_MESSAGE _pRpcMessage,
                                                         DWORD *_pdwStubPhase);


  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkIOControl_SetAnalogueOutput2_Proxy(
                                                                                           IDecklinkIOControl * This,
                                                                                           /* [in] */ unsigned long videoOutput,
                                                                                           /* [in] */ BOOL setupIs75,
                                                                                           /* [in] */ BOOL componentLevelsSMPTE);


  void __RPC_STUB IDecklinkIOControl_SetAnalogueOutput2_Stub(
                                                             IRpcStubBuffer *This,
                                                             IRpcChannelBuffer *_pRpcChannelBuffer,
                                                             PRPC_MESSAGE _pRpcMessage,
                                                             DWORD *_pdwStubPhase);



#endif  /* __IDecklinkIOControl_INTERFACE_DEFINED__ */


#ifndef __IDecklinkMediaSample_INTERFACE_DEFINED__
#define __IDecklinkMediaSample_INTERFACE_DEFINED__

  /* interface IDecklinkMediaSample */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkMediaSample;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("4CAEF6E0-714A-4b4c-902D-BC53AAB2C423")
  IDecklinkMediaSample : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVANCBuffer(
                                                                       /* [out] */ byte **ppBuffer) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkMediaSampleVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkMediaSample * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkMediaSample * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkMediaSample * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetVANCBuffer )(
                                                                    IDecklinkMediaSample * This,
                                                                    /* [out] */ byte **ppBuffer);

    END_INTERFACE
  } IDecklinkMediaSampleVtbl;

  interface IDecklinkMediaSample
  {
    CONST_VTBL struct IDecklinkMediaSampleVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkMediaSample_QueryInterface(This,riid,ppvObject)  \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkMediaSample_AddRef(This)       \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkMediaSample_Release(This)      \
  (This)->lpVtbl -> Release(This)


#define IDecklinkMediaSample_GetVANCBuffer(This,ppBuffer) \
  (This)->lpVtbl -> GetVANCBuffer(This,ppBuffer)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkMediaSample_GetVANCBuffer_Proxy(
                                                                                        IDecklinkMediaSample * This,
                                                                                        /* [out] */ byte **ppBuffer);


  void __RPC_STUB IDecklinkMediaSample_GetVANCBuffer_Stub(
                                                          IRpcStubBuffer *This,
                                                          IRpcChannelBuffer *_pRpcChannelBuffer,
                                                          PRPC_MESSAGE _pRpcMessage,
                                                          DWORD *_pdwStubPhase);



#endif  /* __IDecklinkMediaSample_INTERFACE_DEFINED__ */


  /* interface __MIDL_itf_DecklinkInterface_0391 */
  /* [local] */

  typedef /* [public][v1_enum] */
  enum __MIDL___MIDL_itf_DecklinkInterface_0391_0001
    { DECKLINK_VIDEODESKTOPMODE_DESKTOP = 0,
      DECKLINK_VIDEODESKTOPMODE_LASTFRAME = DECKLINK_VIDEODESKTOPMODE_DESKTOP + 1,
      DECKLINK_VIDEODESKTOPMODE_BLACK = DECKLINK_VIDEODESKTOPMODE_LASTFRAME + 1,
      DECKLINK_VIDEODESKTOPMODE_MAX = DECKLINK_VIDEODESKTOPMODE_BLACK + 1
    }   DECKLINK_VIDEODESKTOPMODE;



  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0391_v0_0_c_ifspec;
  extern RPC_IF_HANDLE __MIDL_itf_DecklinkInterface_0391_v0_0_s_ifspec;

#ifndef __IDecklinkVideoDesktop_INTERFACE_DEFINED__
#define __IDecklinkVideoDesktop_INTERFACE_DEFINED__

  /* interface IDecklinkVideoDesktop */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkVideoDesktop;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("A45C974C-5D26-462f-B5F1-88AA1EFFFF63")
  IDecklinkVideoDesktop : public IUnknown
  {
  public:
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVideoDesktopMode(
                                                                             /* [in] */ unsigned long mode) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkVideoDesktopVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkVideoDesktop * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkVideoDesktop * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkVideoDesktop * This);

    /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetVideoDesktopMode )(
                                                                          IDecklinkVideoDesktop * This,
                                                                          /* [in] */ unsigned long mode);

    END_INTERFACE
  } IDecklinkVideoDesktopVtbl;

  interface IDecklinkVideoDesktop
  {
    CONST_VTBL struct IDecklinkVideoDesktopVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkVideoDesktop_QueryInterface(This,riid,ppvObject) \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkVideoDesktop_AddRef(This)      \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkVideoDesktop_Release(This)     \
  (This)->lpVtbl -> Release(This)


#define IDecklinkVideoDesktop_SetVideoDesktopMode(This,mode)  \
  (This)->lpVtbl -> SetVideoDesktopMode(This,mode)

#endif /* COBJMACROS */


#endif  /* C style interface */



  /* [helpstring] */ HRESULT STDMETHODCALLTYPE IDecklinkVideoDesktop_SetVideoDesktopMode_Proxy(
                                                                                               IDecklinkVideoDesktop * This,
                                                                                               /* [in] */ unsigned long mode);


  void __RPC_STUB IDecklinkVideoDesktop_SetVideoDesktopMode_Stub(
                                                                 IRpcStubBuffer *This,
                                                                 IRpcChannelBuffer *_pRpcChannelBuffer,
                                                                 PRPC_MESSAGE _pRpcMessage,
                                                                 DWORD *_pdwStubPhase);



#endif  /* __IDecklinkVideoDesktop_INTERFACE_DEFINED__ */


#ifndef __IDecklinkReferenceClock_INTERFACE_DEFINED__
#define __IDecklinkReferenceClock_INTERFACE_DEFINED__

  /* interface IDecklinkReferenceClock */
  /* [helpstring][uuid][object] */


  EXTERN_C const IID IID_IDecklinkReferenceClock;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("E2ED66BF-C926-42c0-84BB-9A830F805DDB")
  IDecklinkReferenceClock : public IUnknown
  {
  public:
    virtual HRESULT STDMETHODCALLTYPE GetFrameTime(
                                                   /* [out] */ REFERENCE_TIME *rtFrame) = 0;

  };

#else   /* C style interface */

  typedef struct IDecklinkReferenceClockVtbl
  {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
                                                  IDecklinkReferenceClock * This,
                                                  /* [in] */ REFIID riid,
                                                  /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
                                        IDecklinkReferenceClock * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
                                         IDecklinkReferenceClock * This);

    HRESULT ( STDMETHODCALLTYPE *GetFrameTime )(
                                                IDecklinkReferenceClock * This,
                                                /* [out] */ REFERENCE_TIME *rtFrame);

    END_INTERFACE
  } IDecklinkReferenceClockVtbl;

  interface IDecklinkReferenceClock
  {
    CONST_VTBL struct IDecklinkReferenceClockVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#define IDecklinkReferenceClock_QueryInterface(This,riid,ppvObject) \
  (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDecklinkReferenceClock_AddRef(This)    \
  (This)->lpVtbl -> AddRef(This)

#define IDecklinkReferenceClock_Release(This)   \
  (This)->lpVtbl -> Release(This)


#define IDecklinkReferenceClock_GetFrameTime(This,rtFrame)  \
  (This)->lpVtbl -> GetFrameTime(This,rtFrame)

#endif /* COBJMACROS */


#endif  /* C style interface */



  HRESULT STDMETHODCALLTYPE IDecklinkReferenceClock_GetFrameTime_Proxy(
                                                                       IDecklinkReferenceClock * This,
                                                                       /* [out] */ REFERENCE_TIME *rtFrame);


  void __RPC_STUB IDecklinkReferenceClock_GetFrameTime_Stub(
                                                            IRpcStubBuffer *This,
                                                            IRpcChannelBuffer *_pRpcChannelBuffer,
                                                            PRPC_MESSAGE _pRpcMessage,
                                                            DWORD *_pdwStubPhase);



#endif  /* __IDecklinkReferenceClock_INTERFACE_DEFINED__ */



#ifndef __DecklinkPublicLib_LIBRARY_DEFINED__
#define __DecklinkPublicLib_LIBRARY_DEFINED__

  /* library DecklinkPublicLib */
  /* [version][uuid] */


  EXTERN_C const IID LIBID_DecklinkPublicLib;

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("44A8B5C7-13B6-4211-BD40-35B629D9E6DF")
    DecklinkVideoCaptureFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureProperties;

#ifdef __cplusplus

  class DECLSPEC_UUID("50D29FCF-70ED-4155-9B2A-91F2CE9A86BA")
    DecklinkVideoCaptureProperties;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureProperties2;

#ifdef __cplusplus

  class DECLSPEC_UUID("6296A3EB-08FF-421e-B5AC-6BC834CF4DB6")
    DecklinkVideoCaptureProperties2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("AAA22F7E-5AA0-49d9-8C8D-B52B1AA92EB7")
    DecklinkAudioCaptureFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureProperties;

#ifdef __cplusplus

  class DECLSPEC_UUID("ED4418E7-582D-4759-AE07-8CA7F771427F")
    DecklinkAudioCaptureProperties;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureProperties2;

#ifdef __cplusplus

  class DECLSPEC_UUID("8869832C-FDE3-468e-B0D2-53BF2D59C17A")
    DecklinkAudioCaptureProperties2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("CEB13CC8-3591-45a5-BA0F-20E9A1D72F76")
    DecklinkVideoRenderFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderProperties;

#ifdef __cplusplus

  class DECLSPEC_UUID("98C36C7C-5985-46c4-909C-0EB7BD0C60F7")
    DecklinkVideoRenderProperties;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("19FA8CC3-56CE-46ab-825D-5CE1A39B137A")
    DecklinkAudioRenderFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderProperties;

#ifdef __cplusplus

  class DECLSPEC_UUID("222A4295-E98B-4af2-9063-340E91BE7E68")
    DecklinkAudioRenderProperties;
#endif

  EXTERN_C const CLSID CLSID_DecklinkCaptureFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("472BB322-7639-412e-AF90-F86F1AD6A22F")
    DecklinkCaptureFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkRenderFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("189B7800-82A0-4e92-A2E9-2C8E4A15C3E3")
    DecklinkRenderFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter2;

#ifdef __cplusplus

  class DECLSPEC_UUID("CE3FF814-04C1-4827-9F18-426203E9B1B3")
    DecklinkVideoCaptureFilter2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter3;

#ifdef __cplusplus

  class DECLSPEC_UUID("20722FC4-9EBC-47b9-80B4-6A2ED4D27ECF")
    DecklinkVideoCaptureFilter3;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter4;

#ifdef __cplusplus

  class DECLSPEC_UUID("B3980D18-10D4-4045-9AF7-91EF330AEBF2")
    DecklinkVideoCaptureFilter4;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter5;

#ifdef __cplusplus

  class DECLSPEC_UUID("929CE8E6-96DB-4fe0-980A-83DAB2D2AF31")
    DecklinkVideoCaptureFilter5;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter6;

#ifdef __cplusplus

  class DECLSPEC_UUID("681439FF-EB76-45dd-BF51-80D7C60F5727")
    DecklinkVideoCaptureFilter6;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter7;

#ifdef __cplusplus

  class DECLSPEC_UUID("DE5D8755-421A-43f2-8847-E5A9ACB44D04")
    DecklinkVideoCaptureFilter7;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoCaptureFilter8;

#ifdef __cplusplus

  class DECLSPEC_UUID("976AC924-E89A-4a20-B692-482346E24C71")
    DecklinkVideoCaptureFilter8;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter2;

#ifdef __cplusplus

  class DECLSPEC_UUID("1BCC3EF4-724F-4a45-B61D-8D4BBF32C5BF")
    DecklinkAudioCaptureFilter2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter3;

#ifdef __cplusplus

  class DECLSPEC_UUID("BC90EE8C-F8DC-4776-8DCF-24929C13F9D2")
    DecklinkAudioCaptureFilter3;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter4;

#ifdef __cplusplus

  class DECLSPEC_UUID("1EF51542-B849-4337-B7AE-C980C13F86BF")
    DecklinkAudioCaptureFilter4;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter5;

#ifdef __cplusplus

  class DECLSPEC_UUID("DE6DB48D-66EE-477a-BE4B-522B2253CD95")
    DecklinkAudioCaptureFilter5;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter6;

#ifdef __cplusplus

  class DECLSPEC_UUID("B8E0C8A1-777D-4727-A542-6DF0DF1E1D1C")
    DecklinkAudioCaptureFilter6;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter7;

#ifdef __cplusplus

  class DECLSPEC_UUID("ED1441F5-A132-44e7-9027-14DB5674149B")
    DecklinkAudioCaptureFilter7;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioCaptureFilter8;

#ifdef __cplusplus

  class DECLSPEC_UUID("37BD18AA-F5BD-434c-B0C5-7AE570C377A5")
    DecklinkAudioCaptureFilter8;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter2;

#ifdef __cplusplus

  class DECLSPEC_UUID("16A2E3A5-0C3E-4484-9E78-3ABF2FBE3ACE")
    DecklinkVideoRenderFilter2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter3;

#ifdef __cplusplus

  class DECLSPEC_UUID("4354ED19-BDE1-4083-9BE7-991AF1F0B527")
    DecklinkVideoRenderFilter3;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter4;

#ifdef __cplusplus

  class DECLSPEC_UUID("4546201B-E2B2-4e78-9B1C-DE6406C3BEFA")
    DecklinkVideoRenderFilter4;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter5;

#ifdef __cplusplus

  class DECLSPEC_UUID("4A5FCAED-CAA7-456e-B8EA-608F35A68A0D")
    DecklinkVideoRenderFilter5;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter6;

#ifdef __cplusplus

  class DECLSPEC_UUID("8128230E-8FC1-4af6-BBF1-6C6B6E7C1F16")
    DecklinkVideoRenderFilter6;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter7;

#ifdef __cplusplus

  class DECLSPEC_UUID("6919722B-7329-4c5f-9F68-BA2FE3CF1C77")
    DecklinkVideoRenderFilter7;
#endif

  EXTERN_C const CLSID CLSID_DecklinkVideoRenderFilter8;

#ifdef __cplusplus

  class DECLSPEC_UUID("4C17A259-854D-433f-B087-9AC89238180A")
    DecklinkVideoRenderFilter8;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter2;

#ifdef __cplusplus

  class DECLSPEC_UUID("8D09D460-D361-40bd-A2D0-7E094B4D56FA")
    DecklinkAudioRenderFilter2;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter3;

#ifdef __cplusplus

  class DECLSPEC_UUID("F34B54D3-15B2-4779-8913-64D6E6C67873")
    DecklinkAudioRenderFilter3;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter4;

#ifdef __cplusplus

  class DECLSPEC_UUID("98FC338D-9524-457c-916A-14016AC483A8")
    DecklinkAudioRenderFilter4;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter5;

#ifdef __cplusplus

  class DECLSPEC_UUID("1A06C310-DCD3-4bc3-8BA3-47F8273A509C")
    DecklinkAudioRenderFilter5;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter6;

#ifdef __cplusplus

  class DECLSPEC_UUID("2C7D2EF4-2054-485b-8310-5EE8305955C9")
    DecklinkAudioRenderFilter6;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter7;

#ifdef __cplusplus

  class DECLSPEC_UUID("EF87B072-BCD3-4f4e-A17C-234C73592631")
    DecklinkAudioRenderFilter7;
#endif

  EXTERN_C const CLSID CLSID_DecklinkAudioRenderFilter8;

#ifdef __cplusplus

  class DECLSPEC_UUID("F8CFB51B-ADE0-4fae-B9A6-38E9F0BB2919")
    DecklinkAudioRenderFilter8;
#endif

  EXTERN_C const CLSID CLSID_DecklinkUpsampleFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("F5C45F6D-E4DD-469d-B397-7341D602C403")
    DecklinkUpsampleFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkEffectsFilter;

#ifdef __cplusplus

  class DECLSPEC_UUID("BFA26F43-FB18-40d9-BD58-5A6CE0F42469")
    DecklinkEffectsFilter;
#endif

  EXTERN_C const CLSID CLSID_DecklinkEffectsProperties;

#ifdef __cplusplus

  class DECLSPEC_UUID("EA131320-64CC-4f3f-B79D-41A383A65EDE")
    DecklinkEffectsProperties;
#endif

#ifndef __MEDIASUBTYPE_V210_INTERFACE_DEFINED__
#define __MEDIASUBTYPE_V210_INTERFACE_DEFINED__

  /* interface MEDIASUBTYPE_V210 */
  /* [auto_handle][version][uuid] */


  EXTERN_C const IID IID_MEDIASUBTYPE_V210;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("30313276-B0B0-4dd3-8E8C-572692D526F6")
  MEDIASUBTYPE_V210
  {
  public:
    BEGIN_INTERFACE
      END_INTERFACE
      };

#else   /* C style interface */

  typedef struct MEDIASUBTYPE_V210Vtbl
  {
    BEGIN_INTERFACE

    END_INTERFACE
  } MEDIASUBTYPE_V210Vtbl;

  interface MEDIASUBTYPE_V210
  {
    CONST_VTBL struct MEDIASUBTYPE_V210Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __MEDIASUBTYPE_V210_INTERFACE_DEFINED__ */


#ifndef __MEDIASUBTYPE_v210a_INTERFACE_DEFINED__
#define __MEDIASUBTYPE_v210a_INTERFACE_DEFINED__

  /* interface MEDIASUBTYPE_v210a */
  /* [auto_handle][version][uuid] */


  EXTERN_C const IID IID_MEDIASUBTYPE_v210a;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("30313276-0000-0010-8000-00AA00389B71")
  MEDIASUBTYPE_v210a
  {
  public:
    BEGIN_INTERFACE
      END_INTERFACE
      };

#else   /* C style interface */

  typedef struct MEDIASUBTYPE_v210aVtbl
  {
    BEGIN_INTERFACE

    END_INTERFACE
  } MEDIASUBTYPE_v210aVtbl;

  interface MEDIASUBTYPE_v210a
  {
    CONST_VTBL struct MEDIASUBTYPE_v210aVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __MEDIASUBTYPE_v210a_INTERFACE_DEFINED__ */


#ifndef __MEDIASUBTYPE_r210_INTERFACE_DEFINED__
#define __MEDIASUBTYPE_r210_INTERFACE_DEFINED__

  /* interface MEDIASUBTYPE_r210 */
  /* [auto_handle][version][uuid] */


  EXTERN_C const IID IID_MEDIASUBTYPE_r210;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("30313272-0000-0010-8000-00AA00389B71")
  MEDIASUBTYPE_r210
  {
  public:
    BEGIN_INTERFACE
      END_INTERFACE
      };

#else   /* C style interface */

  typedef struct MEDIASUBTYPE_r210Vtbl
  {
    BEGIN_INTERFACE

    END_INTERFACE
  } MEDIASUBTYPE_r210Vtbl;

  interface MEDIASUBTYPE_r210
  {
    CONST_VTBL struct MEDIASUBTYPE_r210Vtbl *lpVtbl;
  };



#ifdef COBJMACROS


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __MEDIASUBTYPE_r210_INTERFACE_DEFINED__ */


#ifndef __MEDIASUBTYPE_HDYC_INTERFACE_DEFINED__
#define __MEDIASUBTYPE_HDYC_INTERFACE_DEFINED__

  /* interface MEDIASUBTYPE_HDYC */
  /* [auto_handle][version][uuid] */


  EXTERN_C const IID IID_MEDIASUBTYPE_HDYC;

#if defined(__cplusplus) && !defined(CINTERFACE)

  MIDL_INTERFACE("43594448-0000-0010-8000-00AA00389B71")
  MEDIASUBTYPE_HDYC
  {
  public:
    BEGIN_INTERFACE
      END_INTERFACE
      };

#else   /* C style interface */

  typedef struct MEDIASUBTYPE_HDYCVtbl
  {
    BEGIN_INTERFACE

    END_INTERFACE
  } MEDIASUBTYPE_HDYCVtbl;

  interface MEDIASUBTYPE_HDYC
  {
    CONST_VTBL struct MEDIASUBTYPE_HDYCVtbl *lpVtbl;
  };



#ifdef COBJMACROS


#endif /* COBJMACROS */


#endif  /* C style interface */




#endif  /* __MEDIASUBTYPE_HDYC_INTERFACE_DEFINED__ */

#endif /* __DecklinkPublicLib_LIBRARY_DEFINED__ */

  /* Additional Prototypes for ALL interfaces */

  /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
