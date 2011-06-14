#ifndef OPENEQ_PLATFORM_H
#define OPENEQ_PLATFORM_H

#include <GL/glew.h>
#include <cstdio>
#include <string>
#include <inttypes.h>

/*
// GL_ARB_shader_objects / OpenGL 2.0
#define glCreateShader(...) 0
#define glShaderSource(...)
#define glCompileShader(...)
#define glCreateProgram(...) 0
#define glAttachShader(...)
#define glLinkProgram(...)
#define glUseProgram(...)
#define glGetUniformLocation(...) -1
#define glUniform1f(...)
#define glUniform1i(...)
#define glUniform4fv(...)
#define glUniformMatrix4fv(...)
// GL_ARB_vertex_shader
#define glGetAttribLocation(...) -1
#define glEnableVertexAttribArray(...)
#define glDisableVertexAttribArray(...)
#define glVertexAttribPointer(...)
// GL_ARB_vertex_buffer_object
#define glGenBuffers(...)
#define glBindBuffer(...)
#define glBufferData(...)
#define glDeleteBuffers(...)
*/

char *loadFileData(std::string path);
void freeFileData(char *data);
double currentTime();

#endif
