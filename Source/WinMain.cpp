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
        0.f, 0.75f, 0.f,    1.f, 0.f, 0.f,
        -0.5f, 0.f, 0.f,    0.f, 1.f, 0.f,
        0.5f, 0.f, 0.f,     0.f, 0.f, 1.f
    };

    const char* vertex_shader_source =
        "#version 460 core\n"
        "layout (location = 0) in vec3 vertex_pos;\n"
        "layout (location = 1) in vec3 vertex_color;\n"
        "out vec3 color;\n"
        "void main()\n"
        "{\n"
        "color = vertex_color;\n"
        "gl_Position = vec4(vertex_pos, 1.0);\n"
        "}\n";

    GLCall(GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER));
    GLCall(glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL));
    GLCall(glCompileShader(vertex_shader));

    GLint is_compiled;
    GLchar info_log[1024];
    GLCall(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &is_compiled));
    if (!is_compiled)
    {
        GLCall(glGetShaderInfoLog(vertex_shader, 1024, NULL, info_log));
        std::ostringstream oss;
        oss << "OpenGL Error: Failed to compile vertex shader!" << std::endl << info_log;
        exit(1);
    }

    const char* fragment_shader_source =
        "#version 460 core\n"
        "in vec3 color;\n"
        "out vec4 frag_color;\n"
        "void main()\n"
        "{\n"
        "frag_color = vec4(color, 1.0);\n"
        "}\n";

    GLCall(GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));
    GLCall(glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL));
    GLCall(glCompileShader(fragment_shader));

    GLCall(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &is_compiled));
    if (!is_compiled)
    {
        GLCall(glGetShaderInfoLog(fragment_shader, 1024, NULL, info_log));
        std::ostringstream oss;
        oss << "OpenGL Error: Failed to compile fragment shader!" << std::endl << info_log;
        exit(1);
    }

    GLCall(GLuint shader_program = glCreateProgram());
    GLCall(glAttachShader(shader_program, vertex_shader));
    GLCall(glAttachShader(shader_program, fragment_shader));

    GLCall(glLinkProgram(shader_program));

    GLint is_linked;
    GLCall(glGetProgramiv(shader_program, GL_LINK_STATUS, &is_linked));
    if (!is_linked)
    {
        GLCall(glGetProgramInfoLog(shader_program, 1024, NULL, info_log));
        std::ostringstream oss;
        oss << "OpenGL Error: Failed to link shader program!" << std::endl << info_log;
        exit(1);
    }

    GLCall(glDeleteShader(vertex_shader));
    GLCall(glDeleteShader(fragment_shader));

    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLsizei stride = 6 * sizeof(f32);

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0));
    GLCall(glEnableVertexAttribArray(0));

    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(f32))));
    GLCall(glEnableVertexAttribArray(1));

    GLCall(glUseProgram(shader_program));
    
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