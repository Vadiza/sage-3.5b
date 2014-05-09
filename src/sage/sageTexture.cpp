/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageTexture.cpp
 * Author : Luc Renambot
 *
 * Copyright (C) 2012 Electronic Visualization Laboratory,
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
 * Direct questions, comments etc about SAGE to renambot@uic.edu
 * http://sagecommons.org/
 *
 *****************************************************************************/

#if defined(GLSL_YUV) || defined(SAGE_S3D)
#if !defined(WIN32)
#define GLEW_STATIC 1
#endif
#include <GL/glew.h>
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <fcntl.h>
//extern GLhandleARB PHandle;
#else
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#endif

#include "sageTexture.h"
#include "sageBlock.h"
#include "sageShader.h"

#if defined(SAGE_S3D)
extern GLhandleARB threedPHandle;
#endif


// Use the PBO by default (or not)
#define GLOBAL_PBO_USAGE 1

uint64_t pixel_bytes = 0;

#if defined(WIN32)
#define glGetProcAddress(n) wglGetProcAddress(n)
#if ! defined(GLSL_YUV)
typedef void (*PFNGLCOMPRESSEDTEXIMAGE2DARBPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (*PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
#endif
#if !defined(WIN32)
static PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2D    = 0;
static PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2D = 0;
#endif
#else
#if !defined(GLSL_YUV)
//extern "C" void glCompressedTexImage2D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
//extern "C" void glCompressedTexSubImage2D (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *);
#endif
#endif



sageTexture::sageTexture(int _w, int _h, sagePixFmt _t) :
  texWidth(_w), texHeight(_h), texture(NULL), texHandle(-1), pixelType(_t), target(GL_TEXTURE_2D)
{
  //SAGE_PRINTLOG("Creating a sageTexture: type %d", pixelType);

  // Use or not Pixel Buffer Objects (OpenGL)
  usePBO = GLOBAL_PBO_USAGE;

  // Reset PBO values
  pboIds[0] = -1;
  pboIds[1] = -1;
  pindex    =  0;

  if (usePBO) {
    //target = GL_TEXTURE_RECTANGLE_ARB;
  }
}

sageTexture::~sageTexture()
{
  //SAGE_PRINTLOG("sageTexture destructor");
}

void
sageTexture::init(sageRect &viewPort, sageRect &blockLayout, sageRotation orientation)
{
  texCoord = viewPort;
  texCoord.setOrientation(orientation);
  texInfo = blockLayout;
  texInfo.x = texInfo.y = 0;
}

void
sageTexture::genTexCoord()
{
#if defined(GLSL_YUV) || defined(SAGE_S3D)
  if (pixelType == PIXFMT_YUV ) { // || pixelType == PIXFMT_RGBS3D) {
    texCoord.updateBoundary();
  }
  else {
    sageRect texRect(0, 0, texWidth, texHeight);
    texCoord /= texRect;
  }
#else
  sageRect texRect(0, 0, texWidth, texHeight);
  char msg[TOKEN_LEN];
  texCoord.sprintRect(msg);
  texCoord /= texRect;
#endif
}



void sageTexture::uploadTexture()
{
  if (usePBO) {
    int nextIndex = 0; // pbo index used for next frame

    // increment current index first then get the next index
    // "pindex" is used to copy pixels from a PBO to a texture object
    // "nextIndex" is used to update pixels in a PBO
    pindex = (pindex + 1) % 2;
    nextIndex = (pindex + 1) % 2; // dual-pbo mode
    //nextIndex = pindex;  // single-pbo mode

    // bind the texture and PBO
    glDisable(GL_TEXTURE_2D);
    glEnable(target);
    glBindTexture(target, texHandle);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[pindex]);

    // copy pixels from PBO to texture object
    if (pixelType == PIXFMT_DXT || pixelType == PIXFMT_DXT5 || pixelType == PIXFMT_DXT5YCOCG) {
      glCompressedTexSubImage2D(target, 0, 0, 0, texWidth, texHeight, pInfo.pixelFormat,
                                texWidth * texHeight * bpp, NULL);
    }
    else {
      glTexSubImage2D(target, 0, 0, 0, texWidth, texHeight, pInfo.pixelFormat,
                      pInfo.pixelDataType, NULL);
    }

    // bind PBO to update pixel values
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[nextIndex]);

    // map the buffer object into client's memory
    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // for GPU to finish its job. To avoid waiting (stall), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.

    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth * texHeight * bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr)
    {
      // update data directly on the mapped buffer
      memcpy(ptr, texture, texWidth * texHeight * bpp);
      pixel_bytes += texWidth * texHeight * bpp; // luc
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
    }

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations behave normal ways.
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glDisable(target);
    glEnable(GL_TEXTURE_2D);
  }
}


void sageTexture::deleteTexture()
{
  if (texHandle >= 0) {
    glDeleteTextures(1, &texHandle);
    if (texture)
      free(texture);
    if (usePBO)
      glDeleteBuffersARB(2, pboIds);
  }
}



////////////////////////////////////////////////////////////////////////////////
// Implementation of specific texture classes
////////////////////////////////////////////////////////////////////////////////

GLhandleARB sageTextureYUV::programHandleYUV = 0;
GLhandleARB sageTextureS3DRGB::programHandleS3DRGB = 0;
GLhandleARB sageTextureDXT::programHandleDXT5YCOCG = 0;

////////////////////////////////////////////////////////////////////////////////

static const GLchar *generic_vertex = "          \
\n#extension GL_ARB_texture_rectangle : enable\n \
void main() {                                    \
  gl_TexCoord[0] = gl_MultiTexCoord0;          \
  gl_Position = ftransform();                  \
}";

////////////////////////////////////////////////////////////////////////////////

static const GLchar *dxt5ycocg_fragment = "                         \
uniform sampler2D image;                                            \
void main()                                                         \
{                                                                   \
       vec4 color;                                                  \
       float Co;                                                    \
       float Cg;                                                    \
       float Y;                                                     \
       float scale;                                                 \
       color = texture2D(image, gl_TexCoord[0].xy);                 \
       scale = (color.z * (255.0 / 8.0)) + 1.0;                     \
       Co = (color.x - (0.5 * 256.0 / 255.0)) / scale;              \
       Cg = (color.y - (0.5 * 256.0 / 255.0)) / scale;              \
       Y = color.w;                                                 \
       gl_FragColor = vec4(Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0);  \
}";

////////////////////////////////////////////////////////////////////////////////

static const GLchar *yuv_fragment = "                 \
\n#extension GL_ARB_texture_rectangle : enable\n      \
uniform sampler2DRect yuvtex;                         \
void main(void) {                                     \
    float nx, ny;                                     \
    float y,u,v;                                      \
    float r,g,b;                                      \
    int m, n;                                         \
    vec4 vy, uy;                                      \
    nx=gl_TexCoord[0].x;                              \
    ny=gl_TexCoord[0].y;                              \
    if(mod(floor(nx),2.0)>0.5) {                      \
    uy = texture2DRect(yuvtex,vec2(nx-1.0,ny));   \
    vy = texture2DRect(yuvtex,vec2(nx,ny));       \
    u  = uy.x;                                    \
    v  = vy.x;                                    \
    y  = vy.a;                                    \
    }                                                 \
    else {                                            \
    uy = texture2DRect(yuvtex,vec2(nx,ny));       \
    vy = texture2DRect(yuvtex,vec2(nx+1.0,ny));   \
    u  = uy.x;                                    \
    v  = vy.x;                                    \
    y  = uy.a;                                    \
    }                                                 \
  y = 1.1640625 * ( y - 0.0625);                    \
  u = u - 0.5;                                      \
  v = v - 0.5;                                      \
  r = y + 1.59765625 * v ;                          \
  g = y - 0.390625   * u - 0.8125 * v ;             \
  b = y + 2.015625   * u ;                          \
  gl_FragColor=vec4(r,g,b,1.0);                     \
}";

////////////////////////////////////////////////////////////////////////////////


sageTextureYUV::sageTextureYUV(int _w, int _h, sagePixFmt _t)
  : sageTexture(_w,_h,_t)
{
  if (programHandleYUV==0) {
    // Load the shader
    programHandleYUV = GLSLinstallShaders(generic_vertex, yuv_fragment);

    // Finally, use the program
    glUseProgramObjectARB(programHandleYUV);

    // Switch back to no shader
    glUseProgramObjectARB(0);

    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  }

  pInfo.initType(pixelType);
  bpp = pInfo.bytesPerPixel;
#if (!defined(__APPLE__) && !defined(GLSL_YUV))
  // If no shader and not apple hardware, we have to convert YUV to RGB on CPU
  //    so data size is now 3 bytes
  bpp = 3.0;
#endif

  // No PBO support yet for YUV
  usePBO = 0;
}

////////////////////////////////////////////////////////////////////////////////

sageTextureRGB::sageTextureRGB(int _w, int _h, sagePixFmt _t)
  : sageTexture(_w,_h,_t)
{
  pInfo.initType(pixelType);
  bpp = pInfo.bytesPerPixel;
}

////////////////////////////////////////////////////////////////////////////////

sageTextureDXT::sageTextureDXT(int _w, int _h, sagePixFmt _t)
  : sageTexture(_w,_h,_t)
{
  pInfo.initType(pixelType);
  if (pixelType == PIXFMT_DXT)
    bpp = 0.5;
  if (pixelType == PIXFMT_DXT5 || pixelType == PIXFMT_DXT5YCOCG)
    bpp = 1.0;
  if (pixelType == PIXFMT_DXT5YCOCG) {
    if (programHandleDXT5YCOCG==0) {
      // Load the shader
      programHandleDXT5YCOCG = GLSLinstallShaders(generic_vertex, dxt5ycocg_fragment);

      // Finally, use the program
      glUseProgramObjectARB(programHandleDXT5YCOCG);

      // Switch back to no shader
      glUseProgramObjectARB(0);

      GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static const GLchar *s3drgb_fragment = "                  \
uniform sampler2D tex0;        \
uniform sampler2D tex1;        \
void main(void)                                         \
{                                                       \
    /* Get default texel read position (x,y): x is column, y is row of image. */  \
    vec2 pos = gl_TexCoord[0].st; \
    /* oddrow == 1 for odd lines, 0 for even lines: */ \
    float oddrow = mod(pos.y, 2.0); \
    /* Update the y component to only read upper half of all lines in images: */ \
    /* pos.y = (pos.y - oddrow) / 2.0; */ \
    /* Read the image pixel from the modified readpos, output it to framebuffer. */ \
    /* Choose between left- and right- image input buffer, depending if this is  */ \
    /* an even or an odd output row: */ \
    if ( ( (gl_FragCoord/2.0 - vec4(ivec4(gl_FragCoord.y/2.0)))).y < 0.5) { \
        /* Even row: Left image */ \
        gl_FragColor = texture2D(tex1, pos); \
    } \
    else { \
        /* Odd row: Right image */ \
        gl_FragColor = texture2D(tex0, pos); \
    } \
}";



// vec4 s1 = texture2D(tex0, gl_TexCoord[0].st);
// vec4 s2 = texture2D(tex1, gl_TexCoord[0].st);
// gl_FragColor = vec4(s2.r,s2.g,s2.b,1.0);

////////////////////////////////////////////////////////////////////////////////

sageTextureS3DRGB::sageTextureS3DRGB(int _w, int _h, sagePixFmt _t)
  : sageTexture(_w,_h,_t)
{
  if (programHandleS3DRGB==0) {
    // Load the shader
    programHandleS3DRGB = GLSLinstallShaders(generic_vertex, s3drgb_fragment);

    // Finally, use the program
    glUseProgramObjectARB(programHandleS3DRGB);

    // Switch back to no shader
    glUseProgramObjectARB(0);

    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  }

  pInfo.initType(pixelType);
  bpp = 3; //bpp = pInfo.bytesPerPixel;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
sageTextureRGB::renewTexture()
{
  //generate the textures which will be used
  int newWidth, newHeight;
  newWidth = texInfo.width; // getMax2n(texInfo.width);
  newHeight = texInfo.height; // getMax2n(texInfo.height);

  int maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&maxt);
  if (newWidth > maxt)  {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d", newWidth, maxt);
    newWidth = maxt;
  }
  if (newHeight > maxt) {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d", newHeight, maxt);
    newHeight = maxt;
  }

  if (newWidth <= texWidth && newHeight <= texHeight) {
    //std::cout << "reuse texture : " << texWidth << " , " << texHeight);
    return;
  }

  texWidth = newWidth;
  texHeight = newHeight;

  //SAGE_PRINTLOG("Initializing a sageTexture: %dx%d type %d", texWidth, texHeight, pixelType);

  deleteTexture();

  GLuint handle;
  glGenTextures(1, &handle);
  texHandle = handle;

  // Create GL texture object
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Create dummy texture image.
  texture = (GLubyte *) malloc(texWidth * texHeight * bpp);
  memset(texture, 0, texWidth * texHeight * bpp);

  glEnable(target);
  glBindTexture(target, handle);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(target, 0, pInfo.internalFormat, texWidth, texHeight, 0,
               pInfo.pixelFormat, pInfo.pixelDataType, texture);

  if (usePBO) {
    // create two pixel buffer objects
    // glBufferDataARB with NULL pointer reserves only memory space.
    glGenBuffersARB(2, pboIds);

    // First PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texWidth*texHeight*bpp);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Second PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texWidth*texHeight*bpp);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Bind to default buffer
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    pindex = 0;
  }
}

void
sageTextureYUV::renewTexture()
{
  //generate the textures which will be used
  int newWidth, newHeight;
  newWidth = texInfo.width; // getMax2n(texInfo.width);
  newHeight = texInfo.height; // getMax2n(texInfo.height);

  int maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&maxt);
  if (newWidth > maxt)  {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d\n", newWidth, maxt);
    newWidth = maxt;
  }
  if (newHeight > maxt) {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d\n", newHeight, maxt);
    newHeight = maxt;
  }

  if (newWidth <= texWidth && newHeight <= texHeight) {
    //std::cout << "reuse texture : " << texWidth << " , " << texHeight);
    return;
  }

  texWidth = newWidth;
  texHeight = newHeight;

  //SAGE_PRINTLOG("Initializing a sageTexture: %dx%d type %d", texWidth, texHeight, pixelType);

  deleteTexture();

  GLuint handle;
  glGenTextures(1, &handle);
  texHandle = handle;

  // Create GL texture object
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#if defined(GLSL_YUV)
  texture = (GLubyte *) malloc(texWidth * texHeight * bpp);
  memset(texture, 0, texWidth * texHeight * bpp);

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, handle);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, pInfo.internalFormat, texWidth, texHeight, 0,
               pInfo.pixelFormat, pInfo.pixelDataType, texture);
#else
  texture = (GLubyte *) malloc(texWidth * texHeight * 3);
  memset(texture, 0, texWidth * texHeight * 3);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
#endif
}

void
sageTextureDXT::renewTexture()
{
  //generate the textures which will be used
  int newWidth, newHeight;
  newWidth = texInfo.width; // getMax2n(texInfo.width);
  newHeight = texInfo.height; // getMax2n(texInfo.height);

  int maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&maxt);
  if (newWidth > maxt)  {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d\n", newWidth, maxt);
    newWidth = maxt;
  }
  if (newHeight > maxt) {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d\n", newHeight, maxt);
    newHeight = maxt;
  }

  if (newWidth <= texWidth && newHeight <= texHeight) {
    //std::cout << "reuse texture : " << texWidth << " , " << texHeight);
    return;
  }

  texWidth = newWidth;
  texHeight = newHeight;

  //SAGE_PRINTLOG("Initializing a sageTexture: %dx%d type %d", texWidth, texHeight, pixelType);

  deleteTexture();

  GLuint handle;
  glGenTextures(1, &handle);
  texHandle = handle;

  // Create GL texture object
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Create dummy texture image.
  if (texWidth < 64) texWidth = 64;
  if (texHeight < 64) texHeight = 64;

  //texture = (GLubyte *) malloc(texWidth * texHeight / 2);
  //memset(texture, 0, texWidth * texHeight / 2);
  texture = (GLubyte *) malloc(texWidth * texHeight * bpp);
  memset(texture, 0, texWidth * texHeight * bpp);

  glEnable(target);
  glBindTexture(target, handle);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glCompressedTexImage2D(target, 0,
                         pInfo.pixelFormat, texWidth, texHeight, 0,
                         texWidth*texHeight*bpp, texture);

  if (usePBO) {
    // create two pixel buffer objects
    // glBufferDataARB with NULL pointer reserves only memory space.
    glGenBuffersARB(2, pboIds);

    // First PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texWidth*texHeight*bpp);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer


    // Second PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texWidth*texHeight*bpp);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Reset to default buffer
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    pindex = 0;
  }

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
}

void
sageTextureS3DRGB::renewTexture()
{
  //generate the textures which will be used
  int newWidth, newHeight;
  newWidth = texInfo.width; // getMax2n(texInfo.width);
  newHeight = texInfo.height; // getMax2n(texInfo.height);

  int maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&maxt);
  if (newWidth > maxt)  {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d", newWidth, maxt);
    newWidth = maxt;
  }
  if (newHeight > maxt) {
    SAGE_PRINTLOG("Warning: trying to create texture %d, bigger than maximun %d", newHeight, maxt);
    newHeight = maxt;
  }

  if (newWidth <= texWidth && newHeight <= texHeight) {
    //std::cout << "reuse texture : " << texWidth << " , " << texHeight);
    return;
  }

  texWidth = newWidth;
  texHeight = newHeight;

  //SAGE_PRINTLOG("Initializing a sageTexture: %dx%d type %d", texWidth, texHeight, pixelType);

  deleteTexture();

  // Create GL texture object
  GLuint handle[2];
  glGenTextures(2, handle);
  texHandle  = handle[0];
  texRHandle = handle[1];

  // Create dummy texture image.
  int texsize = texWidth * texHeight * bpp;
  //SAGE_PRINTLOG("Allocating texture: %d x %d : %d bytes", texWidth, texHeight , texsize);
  texture = (GLubyte *) malloc(texsize);
  memset(texture, 0, texsize);
  textureR = (GLubyte *) malloc(texsize);
  memset(textureR, 0, texsize);

  glActiveTexture(GL_TEXTURE0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glEnable(target);
  glBindTexture(target, texHandle);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(target, 0, pInfo.internalFormat, texWidth, texHeight, 0,
               pInfo.pixelFormat, pInfo.pixelDataType, texture);

  if (usePBO) {
    // create two pixel buffer objects
    // glBufferDataARB with NULL pointer reserves only memory space.
    glGenBuffersARB(2, pboIds);

    // First PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texsize);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Second PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texsize);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Bind to default buffer
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    pindex = 0;
  }

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glActiveTexture(GL_TEXTURE1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glEnable(target);
  glBindTexture(target, texRHandle);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(target, 0, pInfo.internalFormat, texWidth, texHeight, 0,
               pInfo.pixelFormat, pInfo.pixelDataType, textureR);

  if (usePBO) {
    // create two pixel buffer objects
    // glBufferDataARB with NULL pointer reserves only memory space.
    glGenBuffersARB(2, pboIdsR);

    // First PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsR[0]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texsize);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Second PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsR[1]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth*texHeight*bpp, 0, GL_STREAM_DRAW_ARB);
    ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr) {
      // reset data directly on the mapped buffer
      memset(ptr, 0, texsize);
    }
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer

    // Bind to default buffer
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    pindex = 0;
  }
  glActiveTexture(GL_TEXTURE0);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void sageTextureRGB::copyIntoTexture(int _x, int _y, int _width, int _height, char *_block)
{
  GLubyte *destptr;
  char    *srcptr;
  int      i;

  destptr = texture + (int)(_y*texWidth*bpp + _x*bpp);
  srcptr  = _block;
  for (i=0;i<_height;i++) {
    memcpy(destptr , srcptr, (int)(_width*bpp));
    destptr += int(texWidth*bpp);
    srcptr  += int(_width*bpp);
  }
}


inline int format_row_stride(int width)
{
  const GLuint bw = 4;
  const GLuint wblocks = (width + bw - 1) / bw;
  const GLint  stride = wblocks * 8;
  return stride;
}
inline int format_row_stride56(int width)
{
  const GLuint bw = 4;
  const GLuint wblocks = (width + bw - 1) / bw;
  const GLint  stride = wblocks * 16;
  return stride;
}

void sageTextureDXT::copyIntoTexture(int _x, int _y, int _width, int _height, char *_block)
{
  GLubyte *destptr;
  char    *srcptr;
  int      i;

  int srcRowStride, destRowStride;
  if (pixelType == PIXFMT_DXT) {
    srcRowStride  = format_row_stride(_width);
    destRowStride = format_row_stride(texWidth);
  } else {
    srcRowStride  = format_row_stride56(_width);
    destRowStride = format_row_stride56(texWidth);
  }

  destptr = texture + (int)(_y*destRowStride/4 + 4*_x*bpp);
  srcptr  = _block;
  for (i=0;i<_height/4;i++) {
    memcpy(destptr , srcptr, srcRowStride);
    destptr += destRowStride;
    srcptr  += srcRowStride;
  }
}


// To be checked!!!
void sageTextureYUV::copyIntoTexture(int _x, int _y, int _width, int _height, char *_block)
{
  GLubyte *destptr;
  char    *srcptr;
  int      i;

  destptr = texture + (int)(_y*texWidth*bpp + _x*bpp);
  srcptr  = _block;
  for (i=0;i<_height;i++) {
    memcpy(destptr , srcptr, (int)(_width*bpp));
    destptr += int(texWidth*bpp);
    srcptr  += int(_width*bpp);
  }
}


void sageTextureS3DRGB::copyIntoTexture(int _x, int _y, int _width, int _height, char *_block)
{
  GLubyte *destptr;
  GLubyte *destptrR;
  char    *srcptr;
  int      i, j;

  destptr  = texture  + (int)(_y*texWidth*bpp + _x*bpp);
  destptrR = textureR + (int)(_y*texWidth*bpp + _x*bpp);
  srcptr   = _block;

  //SAGE_PRINTLOG("Copying into %4d x %4d  tex %d x %d block %d x %d - bpp %g", _x, _y, texWidth, texHeight, _width, _height, bpp);

  for (i=0;i<_height;i++) {
    for (j=0;j<_width;j++) {
      destptr[int(j*bpp + 0)] = srcptr[int(j*bpp*2 + 0)];
      destptr[int(j*bpp + 1)] = srcptr[int(j*bpp*2 + 1)];
      destptr[int(j*bpp + 2)] = srcptr[int(j*bpp*2 + 2)];

      destptrR[int(j*bpp + 0)] = srcptr[int(j*bpp*2 + 3)];
      destptrR[int(j*bpp + 1)] = srcptr[int(j*bpp*2 + 4)];
      destptrR[int(j*bpp + 2)] = srcptr[int(j*bpp*2 + 5)];
    }
    destptr  += int(texWidth*bpp);
    destptrR += int(texWidth*bpp);
    srcptr   += int(_width*bpp*2);
  }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
sageTexture::drawQuad(float depth, int alpha, int tempAlpha,
                      float left, float right, float bottom, float top)
{
  // set the correct alpha value for this app
  // we store alphas in a separate hash as opposed to montages themselves
  // because there are multiple montages per app so we would have to keep
  // track of all of them separately

  if (tempAlpha > -1)
    glColor4ub(255,255,255, tempAlpha);
  else
    glColor4ub(255,255,255, alpha);

  float texX, texY;

  glBegin(GL_QUADS);
  texCoord.genTexCoord(LOWER_LEFT, texX, texY);
  glTexCoord2f(texX, texY);
  glVertex3f(left, bottom, -depth);

  texCoord.genTexCoord(LOWER_RIGHT, texX, texY);
  glTexCoord2f(texX, texY);
  glVertex3f(right, bottom, -depth);

  texCoord.genTexCoord(UPPER_RIGHT, texX, texY);
  glTexCoord2f(texX, texY);
  glVertex3f(right, top, -depth);

  texCoord.genTexCoord(UPPER_LEFT, texX, texY);
  glTexCoord2f(texX, texY);
  glVertex3f(left, top, -depth);
  glEnd();
}

void
sageTextureRGB::draw(float depth, int alpha, int tempAlpha,
                     float left, float right, float bottom, float top)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texHandle);

  drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);
}


void
sageTextureYUV::draw(float depth, int alpha, int tempAlpha,
                     float left, float right, float bottom, float top)
{
#if defined(GLSL_YUV)
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texHandle);
  glUseProgramObjectARB(sageTextureYUV::programHandleYUV);
  glActiveTexture(GL_TEXTURE0);
  int h=glGetUniformLocationARB(sageTextureYUV::programHandleYUV,"yuvtex");
  glUniform1iARB(h,0);  /* Bind yuvtex to texture unit 0 */
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texHandle);
#else
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texHandle);
#endif

  drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);

#if defined(GLSL_YUV)
  glUseProgramObjectARB(0);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glEnable(GL_TEXTURE_2D);
#endif
}

void
sageTextureDXT::draw(float depth, int alpha, int tempAlpha,
                     float left, float right, float bottom, float top)
{
  if (pixelType == PIXFMT_DXT5YCOCG) {

    //glDisable(GL_TEXTURE_2D);
    //glEnable(GL_TEXTURE_RECTANGLE_ARB);
    //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texHandle);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texHandle);

    glUseProgramObjectARB(sageTextureDXT::programHandleDXT5YCOCG);
    glActiveTexture(GL_TEXTURE0);
    int h=glGetUniformLocationARB(sageTextureDXT::programHandleDXT5YCOCG,"image");
    glUniform1iARB(h,0);  /* Bind yuvtex to texture unit 0 */
    //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texHandle);

    drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);

    glUseProgramObjectARB(0);
    //glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glEnable(GL_TEXTURE_2D);

  } else {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texHandle);

    drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);
  }
}

void
sageTextureS3DRGB::draw(float depth, int alpha, int tempAlpha,
                        float left, float right, float bottom, float top)
{
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glDisable(GL_TEXTURE_2D);
  glEnable(target);
  glBindTexture(target, texHandle);
  glUseProgramObjectARB(sageTextureS3DRGB::programHandleS3DRGB);
  glActiveTexture(GL_TEXTURE0);

  int h1=glGetUniformLocationARB(sageTextureS3DRGB::programHandleS3DRGB,"tex0");
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  int h2=glGetUniformLocationARB(sageTextureS3DRGB::programHandleS3DRGB,"tex1");
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glUniform1iARB(h1,0);  /* Bind tex0 to texture unit 0 */
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glUniform1iARB(h2,1);  /* Bind tex1 to texture unit 1 */

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glActiveTexture(GL_TEXTURE0);
  glEnable(target);
  glBindTexture(GL_TEXTURE_2D, texHandle);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glActiveTexture(GL_TEXTURE1);
  glEnable(target);
  glBindTexture(GL_TEXTURE_2D, texRHandle);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glUseProgramObjectARB(0);
  glDisable(target);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  //  glEnable(GL_TEXTURE_2D);
  //  glBindTexture(GL_TEXTURE_2D, texHandle);
  //  drawQuad(depth, alpha, tempAlpha, left, right, bottom, top);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void sageTextureRGB::loadPixelBlock(sagePixelBlock *block)
{
  if (usePBO) {
    copyIntoTexture(block->x, block->y, block->width, block->height, block->getPixelBuffer());
  } else {
    glBindTexture(target, texHandle);
    glTexSubImage2D(target, 0, block->x, block->y, block->width, block->height,
                    pInfo.pixelFormat, pInfo.pixelDataType, block->getPixelBuffer());
    pixel_bytes += block->width * block->height * pInfo.bytesPerPixel;
  }
}


void sageTextureYUV::loadPixelBlock(sagePixelBlock *block)
{
#if !defined(__APPLE__)

#define clip(A) ( ((A)<0) ? 0 : ( ((A)>255) ? 255 : (A) ) )

#define YUV444toRGB888(Y,U,V,R,G,B)                                     \
  R = clip(( 298 * (Y-16)                 + 409 * (V-128) + 128) >> 8); \
  G = clip(( 298 * (Y-16) - 100 * (U-128) - 208 * (V-128) + 128) >> 8); \
  B = clip(( 298 * (Y-16) + 516 * (U-128)                 + 128) >> 8);

#if defined(GLSL_YUV)
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texHandle);
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, block->x, block->y, block->width, block->height,
                  pInfo.pixelFormat, pInfo.pixelDataType, block->getPixelBuffer());
  pixel_bytes += block->width * block->height * pInfo.bytesPerPixel;
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glEnable(GL_TEXTURE_2D);
#else
  unsigned char *yuv = (unsigned char*)block->getPixelBuffer();
  unsigned char u,v,y1,y2;
  unsigned char r1,r2,g1,g2,b1,b2;
  int i, k;
  k = 0;
  for (i=0;i< (block->width*block->height)/2;i++) {
    u  = yuv[4*i+0];
    y1 = yuv[4*i+1];
    v  = yuv[4*i+2];
    y2 = yuv[4*i+3];

    YUV444toRGB888(y1, u, v, r1,g1,b1);
    YUV444toRGB888(y2, u, v, r2,g2,b2);
    texture[k + 0] = r1;
    texture[k + 1] = g1;
    texture[k + 2] = b1;
    texture[k + 3] = r2;
    texture[k + 4] = g2;
    texture[k + 5] = b2;
    k += 6;
  }
  glBindTexture(GL_TEXTURE_2D, texHandle);
  glTexSubImage2D(GL_TEXTURE_2D, 0, block->x, block->y, block->width, block->height,
                  GL_RGB, GL_UNSIGNED_BYTE, texture);
  pixel_bytes += block->width * block->height * pInfo.bytesPerPixel; // luc
#endif

#else
  // default behavior on Mac since there's a YUV texture format
  if (usePBO) {
    copyIntoTexture(block->x, block->y, block->width, block->height, block->getPixelBuffer());
  } else {
    glBindTexture(target, texHandle);
    glTexSubImage2D(target, 0, block->x, block->y, block->width, block->height,
                    pInfo.pixelFormat, pInfo.pixelDataType, block->getPixelBuffer());
    pixel_bytes += block->width * block->height * pInfo.bytesPerPixel;
  }
#endif
}

// DXT: use compressed texture download
// to do: add support for YYCoCg DXT format
void sageTextureDXT::loadPixelBlock(sagePixelBlock *block)
{
  if (usePBO) {
    copyIntoTexture(block->x, block->y, block->width, block->height, block->getPixelBuffer());
  } else {
    glBindTexture(target, texHandle);
    glCompressedTexSubImage2D(target, 0, block->x, block->y,
                              block->width, block->height,
                              pInfo.pixelFormat,
                              (block->width*block->height)/2,
                              block->getPixelBuffer());
    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  }
}

// default implementation: TO CHECK
void sageTextureS3DRGB::loadPixelBlock(sagePixelBlock *block)
{
  if (usePBO) {
    copyIntoTexture(block->x, block->y, block->width, block->height, block->getPixelBuffer());
  } else {
    glBindTexture(target, texHandle);
    glTexSubImage2D(target, 0, block->x, block->y, block->width, block->height,
                    pInfo.pixelFormat, pInfo.pixelDataType, block->getPixelBuffer());
    pixel_bytes += block->width * block->height * pInfo.bytesPerPixel;
  }
}

void sageTextureS3DRGB::uploadTexture()
{
  if (usePBO) {
    int nextIndex = 0; // pbo index used for next frame

    // increment current index first then get the next index
    // "pindex" is used to copy pixels from a PBO to a texture object
    // "nextIndex" is used to update pixels in a PBO
    pindex = (pindex + 1) % 2;
    nextIndex = (pindex + 1) % 2; // dual-pbo mode
    //nextIndex = pindex;  // single-pbo mode

    // bind the texture and PBO
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glEnable(target);
    glBindTexture(target, texHandle);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[pindex]);

    // copy pixels from PBO to texture object
    glTexSubImage2D(target, 0, 0, 0, texWidth, texHeight, pInfo.pixelFormat, pInfo.pixelDataType, NULL);

    // bind PBO to update pixel values
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[nextIndex]);

    // map the buffer object into client's memory
    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // for GPU to finish its job. To avoid waiting (stall), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.

    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth * texHeight * bpp, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr)
    {
      // update data directly on the mapped buffer
      memcpy(ptr, texture, texWidth * texHeight * bpp);
      pixel_bytes += texWidth * texHeight * bpp; // luc
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
    }

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations behave normal ways.
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glDisable(target);
    glEnable(GL_TEXTURE_2D);



    // bind the texture and PBO
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glEnable(target);
    glBindTexture(target, texRHandle);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsR[pindex]);

    // copy pixels from PBO to texture object
    glTexSubImage2D(target, 0, 0, 0, texWidth, texHeight, pInfo.pixelFormat, pInfo.pixelDataType, NULL);

    // bind PBO to update pixel values
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsR[nextIndex]);

    // map the buffer object into client's memory
    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // for GPU to finish its job. To avoid waiting (stall), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.

    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, texWidth * texHeight * bpp, 0, GL_STREAM_DRAW_ARB);
    ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if(ptr)
    {
      // update data directly on the mapped buffer
      memcpy(ptr, textureR, texWidth * texHeight * bpp);
      pixel_bytes += texWidth * texHeight * bpp; // luc
      glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
    }

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations behave normal ways.
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    glDisable(target);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

  }
}
