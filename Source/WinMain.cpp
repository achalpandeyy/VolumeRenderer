#include "Core/Win32.h"
#include "Core/Types.h"
#include "Core/OpenGL.h"

// NOTE(achal): Linking with opengl32.dll makes sure that you don't have to GetProcAddress OpenGL 1.1 or below
// functions from opengl32.dll.
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

    HDC device_context = GetDC(window);
    HGLRC opengl_context = Win32InitOpenGL(device_context);

    GLCall(LPCSTR version = (LPCSTR)glGetString(GL_VERSION));
    SetWindowTextA(window, version);

    f32 vertices[] =
    {
        0.f, 0.5f, 0.f,
        -0.25f, 0.f, 0.f,
        0.25f, 0.f, 0.f
    };

    // TODO(achal): Create shaders!
    const char* vertex_shader_source = "";
    const char* fragment_shader_source = "";

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