#include "Core/Win32.h"
#include "Core/Types.h"
#include "Core/OpenGL.h"
#include "ArcballCamera.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>

// NOTE(achal): Linking with opengl32.dll makes sure that you don't have to GetProcAddress OpenGL 1.1 or below
// functions from opengl32.dll.
#pragma comment(lib, "opengl32.lib")

int width = 1280;
int height = 720;

// TODO: Make this a member of non-platform window class, so it can be retrived by the windows ptr
// (data stored on windows side)
ArcballCamera camera(glm::vec3(0.f, 0.f, 20.f), glm::vec3(0.f), width, height);

// TODO: This seem to come together to form a Win32Mouse
int last_x = width / 2;
int last_y = height / 2;
int wheel = 0;

LRESULT CALLBACK Win32WindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;

        case WM_MOUSEMOVE:
        {
            if (w_param & MK_LBUTTON)
            {
                const POINTS mouse_pos = MAKEPOINTS(l_param);

                if (!(mouse_pos.x == last_x && mouse_pos.y == last_y))
                {
                    // Update view matrix of the camera to incorporate rotation
                    camera.Rotate({ last_x, last_y }, { mouse_pos.x, mouse_pos.y });

                    last_x = mouse_pos.x;
                    last_y = mouse_pos.y;
                }
            }
        } break;

        case WM_LBUTTONDOWN:
        {
            SetCapture(window);

            const POINTS mouse_pos = MAKEPOINTS(l_param);

            last_x = mouse_pos.x;
            last_y = mouse_pos.y;
        } break;

        case WM_LBUTTONUP:
        {
            ReleaseCapture();
        } break;

        case WM_MOUSEWHEEL:
        {
            wheel += GET_WHEEL_DELTA_WPARAM(w_param);

            if (wheel >= WHEEL_DELTA || wheel <= -WHEEL_DELTA)
            {
                // Update projection matrix of the camera to incorporate zooming
                camera.SetFOV(wheel / WHEEL_DELTA);
                wheel %= WHEEL_DELTA;
            }

        } break;

        case WM_KEYDOWN:
        {
            if (w_param == VK_ESCAPE)
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

    RECT client_window_rect = {};
    client_window_rect.right = width;
    client_window_rect.bottom = height;

    DWORD window_style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE;

    if (AdjustWindowRect(&client_window_rect, window_style, FALSE) == 0)
    {
        OutputDebugStringA("Error: AdjustWindowRect failed!\n");
        exit(1);
    }

    return CreateWindowExA(0, window_class.lpszClassName, name, window_style, CW_USEDEFAULT, CW_USEDEFAULT,
        client_window_rect.right - client_window_rect.left, client_window_rect.bottom - client_window_rect.top, 0, 0, instance, 0);
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

template <typename T>
void Lerp(unsigned int x0, unsigned int x1, T* values)
{
    T y0 = values[x0];
    T y1 = values[x1];

    T m = (y1 - y0) / (f32)(x1 - x0);

    for (unsigned int i = x0 + 1; i <= x1 - 1; ++i)
        values[i] = m * (f32)(i - x0) + y0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    HWND window = Win32CreateWindow(width, height, "Volume Renderer", instance);
    if (!window)
    {
        OutputDebugStringA("Error: Failed to create window!\n");
        exit(1);
    }

    HDC device_context = GetDC(window);
    HGLRC opengl_context = Win32InitOpenGL(device_context);

    // GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glViewport(0, 0, width, height));

    GLCall(LPCSTR version = (LPCSTR)glGetString(GL_VERSION));
    SetWindowTextA(window, version);

    GLuint vol_texture;
    GLCall(glGenTextures(1, &vol_texture));

    GLCall(glBindTexture(GL_TEXTURE_3D, vol_texture));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // Read in the .raw file
    std::ifstream file("../Resources/skull_256x256x256_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        std::streampos pos = file.tellg();
        char* volume_data = new char[pos]; // position of the last character == size of the file
        file.seekg(0, std::ios::beg);
        file.read(volume_data, pos);
        file.close();

        // Upload it to the GPU.
        GLCall(glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, 256, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, volume_data));
    }
    else
    {
        OutputDebugStringA("Unable to open file!\n");
        exit(1);
    }

    GLCall(glBindTexture(GL_TEXTURE_3D, 0));

    // Create a transfer function
    // TODO: This texel count makes too much total size on the stack, allocate this on heap
    const size_t transfer_function_texel_count = 1024;
    glm::vec4 transfer_function[transfer_function_texel_count];

    // A scalar between 0 and 1
    double position[11] =
    {
        0.016671299934387207, 0.059375025331974030, 0.10788870000000000,
        0.12416851441241686, 0.30333566665649414, 0.31228730082511902,
        0.37030640244483948, 0.49667409062385559, 0.50001919269561768,
        0.62602716684341431, 0.73614190687361414
    };

    // TODO: The above list of doubles need not be sorted, sort them and assert sorted

    // Map the position to the index in transfer_function
    size_t tf_idx[11];
    for (unsigned int i = 0; i < 11; ++i)
        tf_idx[i] = std::ceil(position[i] * (transfer_function_texel_count - 1));

    transfer_function[tf_idx[0]] = { 0.0, 0.0, 0.0, 0.0 };
    transfer_function[tf_idx[1]] = { 0.0352941193, 0.0352941193, 0.0352941193, 0.000000000 };
    transfer_function[tf_idx[2]] = { 0.0549019612, 0.0549019612, 0.0549019612, 0.000000000 };
    transfer_function[tf_idx[3]] = { 0.0594919622, 0.0594919622, 0.0594919622, 0.784574449 };
    transfer_function[tf_idx[4]] = { 0.196398869, 0.505882382, 0.211871520, 0.739361703 };
    transfer_function[tf_idx[5]] = { 0.0862745121, 0.0862745121, 0.0862745121, 0.396276593 };
    transfer_function[tf_idx[6]] = { 0.113725491, 0.113725491, 0.113725491, 0.000000000 };
    transfer_function[tf_idx[7]] = { 0.196398869, 0.505882382, 0.211871520, 0.348404258 };
    transfer_function[tf_idx[8]] = { 0.120622568, 0.730693519, 0.992156863, 0.928191483 };
    transfer_function[tf_idx[9]] = { 0.729411781, 0.105836578, 0.396841377, 0.308510631 };
    transfer_function[tf_idx[10]] = { 0.196398869, 0.505882382, 0.211871520, 0.260638297 };

    for (unsigned int i = 0; i < 10; ++i)
        Lerp(tf_idx[i], tf_idx[i + 1], transfer_function);

    for (unsigned int i = 0; i < tf_idx[0]; ++i)
        transfer_function[i] = transfer_function[tf_idx[0]];

    for (unsigned int i = tf_idx[10] + 1; i < transfer_function_texel_count; ++i)
        transfer_function[i] = transfer_function[tf_idx[10]];

    // Make it into a 1D texture
    GLuint transfer_function_texture;
    GLCall(glGenTextures(1, &transfer_function_texture));

    GLCall(glActiveTexture(GL_TEXTURE0 + 1));
    GLCall(glBindTexture(GL_TEXTURE_1D, transfer_function_texture));
    GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // Upload it to the GPU
    GLCall(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, transfer_function_texel_count, 0, GL_RGBA, GL_FLOAT, &transfer_function[0]));

    GLCall(glBindTexture(GL_TEXTURE_1D, 0));

#if 0
    f32 vertices[] =
    {
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        0.f, 1.f, 1.f,
        1.f, 1.f, 1.f
    };
#endif

    f32 vertices[] =
    {
        0.f, 0.f, 0.f,  0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,  1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,  0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,  1.f, 1.f, 0.f,
        0.f, 0.f, 1.f,  0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,  1.f, 0.f, 1.f,
        0.f, 1.f, 1.f,  0.f, 1.f, 1.f,
        1.f, 1.f, 1.f,  1.f, 1.f, 1.f
    };

    u32 indices[] = { 0, 1, 4, 5, 7, 1, 3, 0, 2, 4, 6, 7, 2, 3 };

    // Construct Model Matrix (brings to World Space from Model Space)
    glm::vec3 volume_dims(256);
    glm::vec3 spacing(0.01f);
    glm::mat3 basis;
    basis[0] = { volume_dims.x * spacing.x, 0.f, 0.f };
    basis[1] = { 0.f, volume_dims.y * spacing.y, 0.f };
    basis[2] = { 0.f, 0.f, volume_dims.z * spacing.z };

    glm::vec3 offset = -0.5f * (basis[0] + basis[1] + basis[2]);

    glm::mat4 model(1.f);
    model[0] = glm::vec4(basis[0], 0.f);
    model[1] = glm::vec4(basis[1], 0.f);
    model[2] = glm::vec4(basis[2], 0.f);
    model[3] = glm::vec4(offset, 1.f);

    Shader shader("../Source/Shaders/Shader.vs", "../Source/Shaders/Shader.fs");
    
    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    // GLsizei stride = 3 * sizeof(f32);
    GLsizei stride = 6 * sizeof(f32);

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0));
    GLCall(glEnableVertexAttribArray(0));

    GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(3 * sizeof(f32))));
    GLCall(glEnableVertexAttribArray(1));

    GLuint ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    GLCall(glBindVertexArray(0));

    shader.Bind();
    shader.SetUniform3i("volume_dims", 256, 256, 256);
    shader.SetUniform1i("volume", 0);
    shader.SetUniform1i("transfer_function", 1);

    f32 quad_vertices[] =
    {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    Shader screen_shader("../Source/Shaders/ScreenShader.vs", "../Source/Shaders/ScreenShader.fs");

    GLuint quad_vao;
    GLCall(glGenVertexArrays(1, &quad_vao));
    GLCall(glBindVertexArray(quad_vao));

    GLuint quad_vbo;
    GLCall(glGenBuffers(1, &quad_vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, quad_vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW));

    int screen_stride = 4 * sizeof(f32);
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, screen_stride, (const void*)0));

    GLCall(glEnableVertexAttribArray(1));
    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, screen_stride, (const void*)(2 * sizeof(f32))));

    GLuint fbo;
    GLCall(glGenFramebuffers(1, &fbo));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    GLuint tex_color_buffer;
    GLCall(glGenTextures(1, &tex_color_buffer));
    GLCall(glBindTexture(GL_TEXTURE_2D, tex_color_buffer));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));

    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_color_buffer, 0));

    // Create a renderbuffer
    GLuint rbo;
    GLCall(glGenRenderbuffers(1, &rbo));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    GLCall(b32 framebuffer_status = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));
    if (framebuffer_status)
    {
        OutputDebugStringA("NOTE: Framebuffer is complete!\n");
    }

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    screen_shader.Bind();
    screen_shader.SetUniform1i("screen_texture", 0);

    // GLuint exit_point_texture;
    // GLCall(glGenTextures(1, &exit_point_texture));
    // GLCall(glBindTexture)

    while (!Win32WindowShouldQuit())
    {
        // First pass
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glClearColor(0.1f, 0.1f, 0.1f, 1.f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        // Generate Entry and Exit point textures

        shader.Bind();
        glm::mat4 pvm = camera.projection * camera.view * model;
        shader.SetUniformMatrix4fv("pvm", glm::value_ptr(pvm));

        GLCall(glBindVertexArray(vao));

        // GLCall(glActiveTexture(GL_TEXTURE0));
        // GLCall(glBindTexture(GL_TEXTURE_3D, vol_texture));
        // GLCall(glActiveTexture(GL_TEXTURE0 + 1));
        // GLCall(glBindTexture(GL_TEXTURE_1D, transfer_function_texture));

        GLCall(glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0));

        // Second Pass
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCall(glDisable(GL_DEPTH_TEST));
        GLCall(glClearColor(1.f, 1.f, 1.f, 1.f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        screen_shader.Bind();
        GLCall(glBindVertexArray(quad_vao));
        GLCall(glActiveTexture(GL_TEXTURE0));
        GLCall(glBindTexture(GL_TEXTURE_2D, tex_color_buffer));
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));

        SwapBuffers(device_context);
    }

    // NOTE: Windows will automatically delete the context when it exits out of the program.

    return 0;
}