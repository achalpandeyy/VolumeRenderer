#ifndef OPENGL_H

#include "Core/Types.h"

#if 0
#include "Core/Win32.h"

#include <gl/GL.h>
#include <sstream>

// Reference: https://www.khronos.org/registry/OpenGL/api/GL/glext.h

#define WINAPIP WINAPI*

//
// OpenGL constants
//

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_FRAMEBUFFER                    0x8D40
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_R8                             0x8229

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
typedef void (WINAPIP PGLBUFFERDATA)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
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
typedef GLint (WINAPIP PGLGETUNIFORMLOCATION)(GLuint, const GLchar*);
extern PGLGETUNIFORMLOCATION glGetUniformLocation;
typedef void (WINAPIP PGLUNIFORMMATRIX4FV)(GLint, GLsizei, GLboolean, const GLfloat*);
extern PGLUNIFORMMATRIX4FV glUniformMatrix4fv;
typedef void (WINAPIP PGLUNIFORM1I)(GLint, GLint);
extern PGLUNIFORM1I glUniform1i;
typedef void (WINAPIP PGLACTIVETEXTURE)(GLenum);
extern PGLACTIVETEXTURE glActiveTexture;
typedef void (WINAPIP PGLTEXIMAGE3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
extern PGLTEXIMAGE3D glTexImage3D;
typedef void (WINAPIP PGLUNIFORM3I)(GLint location, GLint v0, GLint v1, GLint v2);
extern PGLUNIFORM3I glUniform3i;
typedef void (WINAPIP PGLUNIFORM3F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern PGLUNIFORM3F glUniform3f;
typedef void (WINAPIP PGLGENFRAMEBUFFERS)(GLsizei n, GLuint* framebuffers);
extern PGLGENFRAMEBUFFERS glGenFramebuffers;
typedef void (WINAPIP PGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
extern PGLBINDFRAMEBUFFER glBindFramebuffer;
typedef GLenum (WINAPIP PGLCHECKFRAMEBUFFERSTATUS)(GLenum target);
extern PGLCHECKFRAMEBUFFERSTATUS glCheckFramebufferStatus;
typedef void (WINAPIP PGLFRAMEBUFFERTEXTURE2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern PGLFRAMEBUFFERTEXTURE2D glFramebufferTexture2D;
typedef void (WINAPIP PGLGENRENDERBUFFERS)(GLsizei n, GLuint* renderbuffers);
extern PGLGENRENDERBUFFERS glGenRenderbuffers;
typedef void (WINAPIP PGLBINDRENDERBUFFER)(GLenum target, GLuint renderbuffer);
extern PGLBINDRENDERBUFFER glBindRenderbuffer;
typedef void (WINAPIP PGLRENDERBUFFERSTORAGE)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern PGLRENDERBUFFERSTORAGE glRenderbufferStorage;
typedef void (WINAPIP PGLFRAMEBUFFERRENDERBUFFER)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern PGLFRAMEBUFFERRENDERBUFFER glFramebufferRenderbuffer;

//
// Initialize OpenGL
//

extern HGLRC Win32InitOpenGL(HDC device_context);
#endif

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
