#ifndef OPENGL_H

#include "Core/Types.h"
#include "Core/Win32.h"

#include <gl/GL.h>
#include <sstream>

#define WINAPIP WINAPI*

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4

typedef ptrdiff_t GLsizeiptr;

typedef void (WINAPIP PGLGENVERTEXARRAYS)(GLsizei, GLuint*);
extern PGLGENVERTEXARRAYS glGenVertexArrays;

typedef void (WINAPIP PGLBINDVERTEXARRAY)(GLuint);
extern PGLBINDVERTEXARRAY glBindVertexArray;

typedef void (WINAPIP PGLGENBUFFERS)(GLsizei, GLuint*);
extern PGLGENBUFFERS glGenBuffers;

typedef void (WINAPIP PGLBINDBUFFER)(GLenum, GLuint);
extern PGLBINDBUFFER glBindBuffer;

typedef void (WINAPIP PGLBUFFERDATA)(GLenum, GLsizeiptr, const void*, GLenum);
extern PGLBUFFERDATA glBufferData;

typedef void (WINAPIP PGLVERTEXATTRIBPOINTER)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
extern PGLVERTEXATTRIBPOINTER glVertexAttribPointer;

typedef void (WINAPIP PGLENABLEVERTEXATTRIBARRAY)(GLuint);
extern PGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray;

extern HGLRC Win32InitOpenGL(HDC device_context);

//
// OpenGL debugging stuff.
//

#define Assert(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearErrors();\
x;\
Assert(GLLogCall(#x, __FILE__, __LINE__))

void GLClearErrors();
b32 GLLogCall(const char* function, const char* file, int line);

#define OPENGL_H
#endif
