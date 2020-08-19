#ifndef OPENGL_H

#include "Core/Types.h"
#include "Core/Win32.h"

#include <gl/GL.h>
#include <sstream>

// Reference: https://www.khronos.org/registry/OpenGL/api/GL/glext.h

#define WINAPIP WINAPI*

//
// OpenGL constants
//

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

//
// OpenGL types
//

typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

//
// OpenGL functions
//

// TODO(achal): Adding new functions is cumbersome. Concot some macro magic!

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
typedef GLuint (WINAPIP PGLCREATESHADER)(GLenum);
extern PGLCREATESHADER glCreateShader;
typedef void (WINAPIP PGLSHADERSOURCE)(GLuint, GLsizei, const GLchar* const*, const GLint*);
extern PGLSHADERSOURCE glShaderSource;
typedef void (WINAPIP PGLCOMPILESHADER)(GLuint);
extern PGLCOMPILESHADER glCompileShader;
typedef void (WINAPIP PGLGETSHADERIV)(GLuint, GLenum, GLint*);
extern PGLGETSHADERIV glGetShaderiv;
typedef void (WINAPIP PGLGETSHADERINFOLOG)(GLuint, GLsizei, GLsizei*, GLchar*);
extern PGLGETSHADERINFOLOG glGetShaderInfoLog;
typedef GLuint (WINAPIP PGLCREATEPROGRAM)(void);
extern PGLCREATEPROGRAM glCreateProgram;
typedef void (WINAPIP PGLATTACHSHADER)(GLuint, GLuint);
extern PGLATTACHSHADER glAttachShader;
typedef void (WINAPIP PGLLINKPROGRAM)(GLuint);
extern PGLLINKPROGRAM glLinkProgram;
typedef void (WINAPIP PGLGETPROGRAMIV)(GLuint, GLenum, GLint*);
extern PGLGETPROGRAMIV glGetProgramiv;
typedef void (WINAPIP PGLGETPROGRAMINFOLOG)(GLuint, GLsizei, GLsizei*, GLchar*);
extern PGLGETPROGRAMINFOLOG glGetProgramInfoLog;
typedef void (WINAPIP PGLDELETESHADER)(GLuint);
extern PGLDELETESHADER glDeleteShader;
typedef void (WINAPIP PGLUSEPROGRAM)(GLuint);
extern PGLUSEPROGRAM glUseProgram;

//
// Initialize OpenGL
//

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
