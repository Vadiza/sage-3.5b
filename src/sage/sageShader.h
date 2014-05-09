/******************************************************************************
 * SAGE - Scalable Adaptive Graphics Environment
 *
 * Module: sageShader.h
 * Author : Luc Renambot
 *
 *   Description:   This is the header file for the video display module of SAGE.
 *
 *   Notes   :    Since the display window may receive its many pieces from many servers, we need to pass multiple buffers
 *         to this class. Each of the buffers passed contains a frame which forms a part (or whole) of the image on
 *         the display side.
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
 ***************************************************************************************************************************/

#ifndef SAGE_SHADER_H
#define SAGE_SHADER_H

#define GLSLVertexShader   1
#define GLSLFragmentShader 2

#include <GL/glew.h>

//
// Returns 1 if an OpenGL error occurred, 0 otherwise.
//
int GLprintError(const char *file, int line);

//
// Print out the information log for a shader object
//
void GLSLprintShaderInfoLog(GLuint shader);

//
// Print out the information log for a program object
//
void GLSLprintProgramInfoLog(GLuint program);

//
// Returns the size in bytes of the shader fileName.
// If an error occurred, it returns -1.
//
// File name convention:
//
// <fileName>.vert
// <fileName>.frag
//
int GLSLShaderSize(char *fileName, int shaderType);

//
// Reads a shader from the supplied file and returns the shader in the
// arrays passed in. Returns 1 if successful, 0 if an error occurred.
// The parameter size is an upper limit of the amount of bytes to read.
// It is ok for it to be too big.
//
int GLSLreadShader(char *fileName, int shaderType, char *shaderText, int size);

//
// Allocate memory to hold the source of our shaders.
//
int GLSLreadShaderSource(char *fileName, GLchar **vertexShader, GLchar **fragmentShader);


GLuint GLSLinstallShaders(const GLchar *Vertex, const GLchar *Fragment);


#endif
