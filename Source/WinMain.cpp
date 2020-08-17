#include "Core/Win32.h"
#include "Core/Types.h"

#include <cstdlib>
#include <sstream>
#include <gl/GL.h>

#define Assert(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearErrors();\
x;\
Assert(GLLogCall(#x, __FILE__, __LINE__))

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

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4

typedef ptrdiff_t GLsizeiptr;

typedef void WINAPI GLGenVertexArrays (GLsizei, GLuint*);
typedef void WINAPI GLBindVertexArray (GLuint);
typedef void WINAPI GLGenBuffers (GLsizei, GLuint*);
typedef void WINAPI GLBindBuffer (GLenum, GLuint);
typedef void WINAPI GLBufferData (GLenum, GLsizeiptr, const void*, GLenum);
typedef void WINAPI GLVertexAttribPointer (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void WINAPI GLEnableVertexAttribArray (GLuint);

// TODO(achal): I would like to pop up OS MessageBox when something fails (window registration/creation)
// instead of debug output.

#pragma comment(lib, "opengl32.lib")

LRESULT CALLBACK Win32WindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    // NOTE(achal): You can get your stuff here by retrieving a pointer from HWND. 

    switch (message)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
    }

    return DefWindowProcA(window, message, w_param, l_param);
}

HWND Win32CreateWindow(int width, int height, const char* name, HINSTANCE instance)
{
    WNDCLASSA window_class = {};
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = Win32WindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Volume Renderer Window Class";

    if (!RegisterClassA(&window_class))
    {
        OutputDebugStringA("Error: Failed to register window class!\n");
        exit(1);
    }

    return CreateWindowExA(0, window_class.lpszClassName, name, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, instance, 0);
}

b32 Win32WindowShouldQuit()
{
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
            return true;

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return false;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    int width = 1280;
    int height = 720;
    HWND window = Win32CreateWindow(width, height, "Volume Renderer", instance);
    if (!window)
    {
        OutputDebugStringA("Error: Failed to create window!\n");
        exit(1);
    }

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;     // TODO(achal): Do we need this for a volume renderer?

    HDC device_context = GetDC(window);

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

    GLGenVertexArrays* glGenVertexArrays = (GLGenVertexArrays*)wglGetProcAddress("glGenVertexArrays");
    if (!glGenVertexArrays)
    {
        OutputDebugStringA("Error: Unable to load glGenVertexArrays function!\n");
        exit(1);
    }

    GLBindVertexArray* glBindVertexArray = (GLBindVertexArray*)wglGetProcAddress("glBindVertexArray");
    if (!glBindVertexArray)
    {
        OutputDebugStringA("Error: Unable to load glBindVertexArray function!\n");
    }

    GLGenBuffers* glGenBuffers = (GLGenBuffers*)wglGetProcAddress("glGenBuffers");
    if (!glGenBuffers)
    {
        OutputDebugStringA("Error: Unable to load glGenBuffers function!\n");
    }

    GLBindBuffer* glBindBuffer = (GLBindBuffer*)wglGetProcAddress("glBindBuffer");
    if (!glBindBuffer)
    {
        OutputDebugStringA("Error: Unable to load glBindBuffer function!\n");
    }

    GLBufferData* glBufferData = (GLBufferData*)wglGetProcAddress("glBufferData");
    if (!glBufferData)
    {
        OutputDebugStringA("Error: Unable to load glBufferData function!\n");
    }

    GLVertexAttribPointer* glVertexAttribPointer = (GLVertexAttribPointer*)wglGetProcAddress("glVertexAttribPointer");
    if (!glVertexAttribPointer)
    {
        OutputDebugStringA("Error: Unable to load glVertexAttribPointer function!\n");
    }

    GLEnableVertexAttribArray* glEnableVertexAttribArray = (GLEnableVertexAttribArray*)wglGetProcAddress("glEnableVertexAttribArray");
    if (!glEnableVertexAttribArray)
    {
        OutputDebugStringA("Error: Unable to load glEnableVertexAttribArray function!\n");
    }

    GLCall(LPCSTR version = (LPCSTR)glGetString(GL_VERSION));
    SetWindowTextA(window, version);

    f32 vertices[] =
    {
        0.f, 0.5f, 0.f,
        -0.5f, 0.f, 0.f,
        0.5f, 0.f, 0.f
    };

    // TODO(achal): Create shaders!

    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
    GLCall(glEnableVertexAttribArray(0));
    
    while (!Win32WindowShouldQuit())
    {
        GLCall(glClearColor(0.1f, 0.1f, 0.1f, 1.f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        GLCall(glBindVertexArray(vao));
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 3));
        SwapBuffers(device_context);
    }

    // NOTE(achal): Windows will automatically delete the context when it exits out of the program.

    return 0;
}