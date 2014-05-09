#include "guids.h"

const GUID CLSID_DivxDecompressorCF={0x82CCd3E0, 0xF71A, 0x11D0,
                                     { 0x9f, 0xe5, 0x00, 0x60, 0x97, 0x78, 0xaa, 0xaa}};
const GUID IID_IDivxFilterInterface={0xd132ee97, 0x3e38, 0x4030,
                                     {0x8b, 0x17, 0x59, 0x16, 0x3b, 0x30, 0xa1, 0xf5}};

const GUID CLSID_IV50_Decoder={0x30355649, 0x0000, 0x0010,
                               {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID IID_IBaseFilter={0x56a86895, 0x0ad4, 0x11ce,
                            {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IEnumPins={0x56a86892, 0x0ad4, 0x11ce,
                          {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IFilterGraph={0x56a8689f, 0x0ad4, 0x11ce,
                             {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IEnumMediaTypes={0x89c31040, 0x846b, 0x11ce,
                                {0x97, 0xd3, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID IID_IMemInputPin={0x56a8689d, 0x0ad4, 0x11ce,
                             {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IMemAllocator={0x56a8689c, 0x0ad4, 0x11ce,
                              {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID IID_IMediaSample={0x56a8689a, 0x0ad4, 0x11ce,
                             {0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};

const GUID MEDIATYPE_Video={0x73646976, 0x0000, 0x0010,
                            {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID GUID_NULL={0x0, 0x0, 0x0,
                      {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
const GUID FORMAT_VideoInfo={0x05589f80, 0xc356, 0x11ce,
                             {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID MEDIASUBTYPE_RGB1={0xe436eb78, 0x524f, 0x11ce,
                              {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB4={0xe436eb79, 0x524f, 0x11ce,
                              {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB8={0xe436eb7a, 0x524f, 0x11ce,
                              {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB565={0xe436eb7b, 0x524f, 0x11ce,
                                {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB555={0xe436eb7c, 0x524f, 0x11ce,
                                {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB24={0xe436eb7d, 0x524f, 0x11ce,
                               {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_RGB32={0xe436eb7e, 0x524f, 0x11ce,
                               {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID MEDIASUBTYPE_YUYV={0x56595559, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_IYUV={0x56555949, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_YVU9={0x39555659, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_Y411={0x31313459, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_Y41P={0x50313459, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_YUY2={0x32595559, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_YVYU={0x55595659, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_UYVY={0x59565955, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_Y211={0x31313259, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_YV12={0x32315659, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_I420={0x30323449, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_IF09={0x39304649, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID CLSID_FilterGraph={0xe436ebb3, 0x524f, 0x11ce,
                              {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
const GUID CLSID_MemoryAllocator={0x1e651cc0, 0xb199, 0x11d0,
                                  {0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}};
const GUID IID_DivxHidden={0x598eba01, 0xb49a, 0x11d2,
                           {0xa1, 0xc1, 0x00, 0x60, 0x97, 0x78, 0xaa, 0xaa}};
const GUID IID_Iv50Hidden={0x665a4442, 0xd905, 0x11d0,
                           {0xa3, 0x0e, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

const GUID FORMAT_WaveFormatEx = {0x05589f81, 0xc356, 0x11CE,
                                  {0xBF, 0x01, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A}};
const GUID MEDIATYPE_Audio = {0x73647561, 0x0000, 0x0010,
                              {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
const GUID MEDIASUBTYPE_PCM = {0x00000001, 0x0000, 0x0010,
                               {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
