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

#include "stdafx.h"
#include "Utils2.h"


// This code was also brazenly stolen from the DX9 SDK
// Pass it a file name in wszPath, and it will save the filter graph to that file.
HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath)
{
  const WCHAR wszStreamName[] = L"ActiveMovieGraph";
  HRESULT hr;
  IStorage *pStorage = NULL;

  // First, create a document file which will hold the GRF file
  hr = StgCreateDocfile(
                        wszPath,
                        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        0, &pStorage);
  if(FAILED(hr))
  {
    return hr;
  }

  // Next, create a stream to store.
  IStream *pStream;
  hr = pStorage->CreateStream(
                              wszStreamName,
                              STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                              0, 0, &pStream);
  if (FAILED(hr))
  {
    pStorage->Release();
    return hr;
  }

  // The IPersistStream converts a stream into a persistent object.
  IPersistStream *pPersist = NULL;
  pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersist));
  hr = pPersist->Save(pStream, TRUE);
  pStream->Release();
  pPersist->Release();
  if (SUCCEEDED(hr))
  {
    hr = pStorage->Commit(STGC_DEFAULT);
  }
  pStorage->Release();
  return hr;
}
