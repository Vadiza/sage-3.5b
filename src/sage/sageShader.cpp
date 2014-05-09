/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageShader.cpp
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sageShader.h"

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

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#endif

#include "sageShader.h"
#include "misc.h"

//
// Returns 1 if an OpenGL error occurred, 0 otherwise.
//
int GLprintError(const char *file, int line)
{
  GLenum glErr;
  int    retCode = 0;

  glErr = glGetError();
  while (glErr != GL_NO_ERROR)
  {
    SAGE_PRINTLOG("GLSL> glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
    retCode = 1;
    glErr = glGetError();
  }
  return retCode;
}


//
// Print out the information log for a shader object
//
void GLSLprintShaderInfoLog(GLuint shader)
{
  int infologLength = 0;
  int charsWritten  = 0;
  GLchar *infoLog;

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  if (infologLength > 0)
  {
    infoLog = (GLchar *)malloc(infologLength);
    if (infoLog == NULL)
    {
      SAGE_PRINTLOG("GLSL> ERROR: Could not allocate InfoLog buffer\n");
      exit(1);
    }
    glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
    if (charsWritten>0)
      SAGE_PRINTLOG("GLSL> Shader InfoLog:%s\n", infoLog);
    free(infoLog);
  }
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
}

//
// Print out the information log for a program object
//
void GLSLprintProgramInfoLog(GLuint program)
{
  int infologLength = 0;
  int charsWritten  = 0;
  GLchar *infoLog;

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);

  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors

  if (infologLength > 0)
  {
    infoLog = (GLchar *)malloc(infologLength);
    if (infoLog == NULL)
    {
      SAGE_PRINTLOG("GLSL> ERROR: Could not allocate InfoLog buffer\n");
      exit(1);
    }
    glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);
    if (charsWritten>0)
      SAGE_PRINTLOG("GLSL> Program InfoLog:%s\n", infoLog);
    free(infoLog);
  }
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
}


//
// Returns the size in bytes of the shader fileName.
// If an error occurred, it returns -1.
//
// File name convention:
//
// <fileName>.vert
// <fileName>.frag
//
int
GLSLShaderSize(char *fileName, int shaderType)
{
  int fd;
  char name[256];
  int count = -1;

  memset(name, 0, 256);
  strcpy(name, fileName);

  switch (shaderType)
  {
  case GLSLVertexShader:
    strcat(name, ".vert");
    break;
  case GLSLFragmentShader:
    strcat(name, ".frag");
    break;
  default:
    SAGE_PRINTLOG("GLSL> ERROR: unknown shader file type\n");
    exit(1);
    break;
  }

  //
  // Open the file, seek to the end to find its length
  //
#ifdef WIN32
  fd = _open(name, _O_RDONLY);
  if (fd != -1)
  {
    count = _lseek(fd, 0, SEEK_END) + 1;
    _close(fd);
  }
#else
  fd = open(name, O_RDONLY);
  if (fd != -1)
  {
    count = lseek(fd, 0, SEEK_END) + 1;
    close(fd);
  }
#endif
  return count;
}


//
// Reads a shader from the supplied file and returns the shader in the
// arrays passed in. Returns 1 if successful, 0 if an error occurred.
// The parameter size is an upper limit of the amount of bytes to read.
// It is ok for it to be too big.
//
int
GLSLreadShader(char *fileName, int shaderType, char *shaderText, int size)
{
  FILE *fh;
  char name[100];
  int count;

  strcpy(name, fileName);

  switch (shaderType)
  {
  case GLSLVertexShader:
    strcat(name, ".vert");
    break;
  case GLSLFragmentShader:
    strcat(name, ".frag");
    break;
  default:
    SAGE_PRINTLOG("GLSL> ERROR: unknown shader file type\n");
    exit(1);
    break;
  }

  //
  // Open the file
  //
  fh = fopen(name, "r");
  if (!fh)
    return -1;

  //
  // Get the shader from a file.
  //
  fseek(fh, 0, SEEK_SET);
  count = (int) fread(shaderText, 1, size, fh);
  shaderText[count] = '\0';

  if (ferror(fh))
    count = 0;

  fclose(fh);
  return count;
}



//
// Allocate memory to hold the source of our shaders.
//
int
GLSLreadShaderSource(char *fileName, GLchar **vertexShader, GLchar **fragmentShader)
{
  int vSize, fSize;

  SAGE_PRINTLOG("GLSL> load shader %s\n", fileName);
  if (vertexShader) {
    vSize = GLSLShaderSize(fileName, GLSLVertexShader);

    if (vSize == -1) {
      SAGE_PRINTLOG("GLSL> Cannot determine size of the vertex shader %s\n", fileName);
      return 0;
    }

    *vertexShader = (GLchar *) malloc(vSize);

    //
    // Read the source code
    //
    if (!GLSLreadShader(fileName, GLSLVertexShader, *vertexShader, vSize)) {
      SAGE_PRINTLOG("GLSL> Cannot read the file %s.vert\n", fileName);
      return 0;
    }
  }

  if (fragmentShader) {
    fSize = GLSLShaderSize(fileName, GLSLFragmentShader);

    if (fSize == -1) {
      SAGE_PRINTLOG("GLSL> Cannot determine size of the fragment shader %s\n", fileName);
      return 0;
    }

    *fragmentShader = (GLchar *) malloc(fSize);

    if (!GLSLreadShader(fileName, GLSLFragmentShader, *fragmentShader, fSize)) {
      SAGE_PRINTLOG("GLSL> Cannot read the file %s.frag\n", fileName);
      return 0;
    }
  }

  return 1;
}

GLuint GLSLinstallShaders(const GLchar *Vertex, const GLchar *Fragment)
{
  GLuint VS, FS, Prog;   // handles to objects
  GLint  vertCompiled, fragCompiled;    // status values
  GLint  linked;

  // Create a program object
  Prog = glCreateProgram();

  // Create a vertex shader object and a fragment shader object
  if (Vertex) {
    VS = glCreateShader(GL_VERTEX_SHADER);

    // Load source code strings into shaders
    glShaderSource(VS, 1, &Vertex, NULL);

    // Compile the vertex shader, and print out
    // the compiler log file.

    glCompileShader(VS);
    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
    glGetShaderiv(VS, GL_COMPILE_STATUS, &vertCompiled);
    GLSLprintShaderInfoLog(VS);

    if (!vertCompiled)
      return 0;

    glAttachShader(Prog, VS);
  }

  if (Fragment) {
    FS = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(FS, 1, &Fragment, NULL);

    glCompileShader(FS);
    GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
    glGetShaderiv(FS, GL_COMPILE_STATUS, &fragCompiled);
    GLSLprintShaderInfoLog(FS);

    if (!fragCompiled)
      return 0;

    glAttachShader(Prog, FS);
  }

  // Link the program object and print out the info log
  glLinkProgram(Prog);
  GLprintError(__FILE__, __LINE__);  // Check for OpenGL errors
  glGetProgramiv(Prog, GL_LINK_STATUS, &linked);
  GLSLprintProgramInfoLog(Prog);

  if (!linked)
    return 0;

  return Prog;
}
