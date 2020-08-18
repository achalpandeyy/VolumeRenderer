#include "OpenGL.h"

PGLGENVERTEXARRAYS glGenVertexArrays = NULL;
PGLBINDVERTEXARRAY glBindVertexArray = NULL;
PGLGENBUFFERS glGenBuffers = NULL;
PGLBINDBUFFER glBindBuffer = NULL;
PGLBUFFERDATA glBufferData = NULL;
PGLVERTEXATTRIBPOINTER glVertexAttribPointer = NULL;
PGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray = NULL;

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