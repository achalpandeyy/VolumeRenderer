#include "OpenGL.h"

extern PGLGENVERTEXARRAYS glGenVertexArrays = NULL;
extern PGLBINDVERTEXARRAY glBindVertexArray = NULL;
extern PGLGENBUFFERS glGenBuffers = NULL;
extern PGLBINDBUFFER glBindBuffer = NULL;
extern PGLBUFFERDATA glBufferData = NULL;
extern PGLVERTEXATTRIBPOINTER glVertexAttribPointer = NULL;
extern PGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray = NULL;
extern PGLCREATESHADER glCreateShader = NULL;
extern PGLSHADERSOURCE glShaderSource = NULL;
extern PGLCOMPILESHADER glCompileShader = NULL;
extern PGLGETSHADERIV glGetShaderiv = NULL;
extern PGLGETSHADERINFOLOG glGetShaderInfoLog = NULL;
extern PGLCREATEPROGRAM glCreateProgram = NULL;
extern PGLATTACHSHADER glAttachShader = NULL;
extern PGLLINKPROGRAM glLinkProgram = NULL;
extern PGLGETPROGRAMIV glGetProgramiv = NULL;
extern PGLGETPROGRAMINFOLOG glGetProgramInfoLog = NULL;
extern PGLDELETESHADER glDeleteShader = NULL;
extern PGLUSEPROGRAM glUseProgram = NULL;
extern PGLGETUNIFORMLOCATION glGetUniformLocation = NULL;
extern PGLUNIFORMMATRIX4FV glUniformMatrix4fv = NULL;
extern PGLUNIFORM1I glUniform1i = NULL;
extern PGLACTIVETEXTURE glActiveTexture = NULL;
extern PGLTEXIMAGE3D glTexImage3D = NULL;
extern PGLUNIFORM3I glUniform3i = NULL;
extern PGLUNIFORM3F glUniform3f = NULL;
extern PGLGENFRAMEBUFFERS glGenFramebuffers = NULL;
extern PGLBINDFRAMEBUFFER glBindFramebuffer = NULL;
extern PGLCHECKFRAMEBUFFERSTATUS glCheckFramebufferStatus = NULL;
extern PGLFRAMEBUFFERTEXTURE2D glFramebufferTexture2D = NULL;
extern PGLGENRENDERBUFFERS glGenRenderbuffers = NULL;
extern PGLBINDRENDERBUFFER glBindRenderbuffer = NULL;
extern PGLRENDERBUFFERSTORAGE glRenderbufferStorage = NULL;
extern PGLFRAMEBUFFERRENDERBUFFER glFramebufferRenderbuffer = NULL;

static PROC LoadProc(const char* name)
{
    PROC proc = wglGetProcAddress(name);
    if (!proc)
    {
        std::ostringstream oss;
        oss << "Error: Unable to load " << name << " function!" << std::endl;
        OutputDebugStringA(oss.str().c_str());
        exit(1);
    }
    return proc;
}

extern HGLRC Win32InitOpenGL(HDC device_context)
{
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;     // TODO(achal): Do we need this for a volume renderer?

    int pf_index = ChoosePixelFormat(device_context, &pfd);
    if (pf_index == 0)
    {
        OutputDebugStringA("Error: Failed to choose a pixel format!\n");
        exit(1);
    }

    if (SetPixelFormat(device_context, pf_index, &pfd) == FALSE)
    {
        OutputDebugStringA("Error: Failed to set pixel format!\n");
        exit(1);
    }

    HGLRC opengl_context = wglCreateContext(device_context);
    if (!opengl_context)
    {
        OutputDebugStringA("Error: Failed to create OpenGL context!\n");
        exit(1);
    }

    if (wglMakeCurrent(device_context, opengl_context) == FALSE)
    {
        OutputDebugStringA("Error: Failed to make OpenGL context current!\n");
        exit(1);
    }

    glGenVertexArrays = (PGLGENVERTEXARRAYS)LoadProc("glGenVertexArrays");
    glBindVertexArray = (PGLBINDVERTEXARRAY)LoadProc("glBindVertexArray");
    glGenBuffers = (PGLGENBUFFERS)LoadProc("glGenBuffers");
    glBindBuffer = (PGLBINDBUFFER)LoadProc("glBindBuffer");
    glBufferData = (PGLBUFFERDATA)LoadProc("glBufferData");
    glVertexAttribPointer = (PGLVERTEXATTRIBPOINTER)LoadProc("glVertexAttribPointer");
    glEnableVertexAttribArray = (PGLENABLEVERTEXATTRIBARRAY)LoadProc("glEnableVertexAttribArray");
    glCreateShader = (PGLCREATESHADER)LoadProc("glCreateShader");
    glShaderSource = (PGLSHADERSOURCE)LoadProc("glShaderSource");
    glCompileShader = (PGLCOMPILESHADER)LoadProc("glCompileShader");
    glGetShaderiv = (PGLGETSHADERIV)LoadProc("glGetShaderiv");
    glGetShaderInfoLog = (PGLGETSHADERINFOLOG)LoadProc("glGetShaderInfoLog");
    glCreateProgram = (PGLCREATEPROGRAM)LoadProc("glCreateProgram");
    glAttachShader = (PGLATTACHSHADER)LoadProc("glAttachShader");
    glLinkProgram = (PGLLINKPROGRAM)LoadProc("glLinkProgram");
    glGetProgramiv = (PGLGETPROGRAMIV)LoadProc("glGetProgramiv");
    glGetProgramInfoLog = (PGLGETPROGRAMINFOLOG)LoadProc("glGetProgramInfoLog");
    glDeleteShader = (PGLDELETESHADER)LoadProc("glDeleteShader");
    glUseProgram = (PGLUSEPROGRAM)LoadProc("glUseProgram");
    glGetUniformLocation = (PGLGETUNIFORMLOCATION)LoadProc("glGetUniformLocation");
    glUniformMatrix4fv = (PGLUNIFORMMATRIX4FV)LoadProc("glUniformMatrix4fv");
    glUniform1i = (PGLUNIFORM1I)LoadProc("glUniform1i");
    glActiveTexture = (PGLACTIVETEXTURE)LoadProc("glActiveTexture");
    glTexImage3D = (PGLTEXIMAGE3D)LoadProc("glTexImage3D");
    glUniform3i = (PGLUNIFORM3I)LoadProc("glUniform3i");
    glUniform3f = (PGLUNIFORM3F)LoadProc("glUniform3f");
    glGenFramebuffers = (PGLGENFRAMEBUFFERS)LoadProc("glGenFramebuffers");
    glBindFramebuffer = (PGLBINDFRAMEBUFFER)LoadProc("glBindFramebuffer");
    glCheckFramebufferStatus = (PGLCHECKFRAMEBUFFERSTATUS)LoadProc("glCheckFramebufferStatus");
    glFramebufferTexture2D = (PGLFRAMEBUFFERTEXTURE2D)LoadProc("glFramebufferTexture2D");
    glGenRenderbuffers = (PGLGENRENDERBUFFERS)LoadProc("glGenRenderbuffers");
    glBindRenderbuffer = (PGLBINDRENDERBUFFER)LoadProc("glBindRenderbuffer");
    glRenderbufferStorage = (PGLRENDERBUFFERSTORAGE)LoadProc("glRenderbufferStorage");
    glFramebufferRenderbuffer = (PGLFRAMEBUFFERRENDERBUFFER)LoadProc("glFramebufferRenderbuffer");

    return opengl_context;
}

void GLClearErrors()
{
    while (glGetError() != GL_NO_ERROR);
}

b32 GLLogCall(const char* function, const char* file, int line)
{
    GLenum error = glGetError();
    while (error != GL_NO_ERROR)
    {
        std::ostringstream oss;
        oss << "OpenGL Error: (" << error << "): " << function << std::endl
            << file << " " << line << std::endl;
        OutputDebugStringA(oss.str().c_str());
        return false;
    }
    return true;
}