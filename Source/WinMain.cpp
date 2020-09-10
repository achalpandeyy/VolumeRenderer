#include "Core/Win32.h"
#include "Core/Types.h"
#include "Core/OpenGL.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/stb_image.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#define PI_32 3.141592f

// NOTE(achal): Linking with opengl32.dll makes sure that you don't have to GetProcAddress OpenGL 1.1 or below
// functions from opengl32.dll.
#pragma comment(lib, "opengl32.lib")

f32 theta_x = 0.f;
f32 theta_y = 0.f;
f32 last_x;
f32 last_y;

glm::mat4 model(1.f);

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
                std::ostringstream oss;
                oss << "[" << mouse_pos.x << ", " << mouse_pos.y << "]";
                SetWindowTextA(window, oss.str().c_str());

                f32 dx = mouse_pos.x - last_x;
                f32 dy = mouse_pos.y - last_y;

                theta_x -= 0.005f * dy;

                theta_x = glm::clamp(theta_x, -PI_32 / 2.f, PI_32 / 2.f);

                theta_y -= 0.005f * dx;
                theta_y = std::fmod(theta_y, 2.f * PI_32);

                model = glm::rotate(glm::mat4(1.f), theta_x, glm::vec3(1.f, 0.f, 0.f));
                model = glm::rotate(model, theta_y, glm::vec3(0.f, 1.f, 0.f));

                last_x = mouse_pos.x;
                last_y = mouse_pos.y;
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
            // Implement zoom with mouse wheel.
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

    GLCall(glEnable(GL_DEPTH_TEST));

    GLCall(LPCSTR version = (LPCSTR)glGetString(GL_VERSION));
    SetWindowTextA(window, version);

#if 0
    char* volume_data = nullptr;

    // Read in the .raw file
    std::ifstream file("../Resources/skull_256x256x256_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        std::streampos pos = file.tellg(); // position of the last character == size of the file
        volume_data = new char[pos];
        file.seekg(0, std::ios::beg);
        file.read(volume_data, pos);
        file.close();

        // Write slices for debugging.
        for (size_t slice_idx = 0; slice_idx < 256; ++slice_idx)
        {
            std::ostringstream oss;
            oss << "slices/slice" << slice_idx << ".ppm";

            std::ofstream imageFile(oss.str().c_str());
            imageFile << "P3\n" << 256 << ' ' << 256 << "\n255\n";

            for (size_t y = 0; y < 256; ++y)
            {
                for (size_t x = 0; x < 256; ++x)
                {
                    size_t idx = (slice_idx * 256 * 256) + (y * 256) + x;
                    unsigned int r = static_cast<unsigned int>(volume_data[idx]);
                    unsigned int g = static_cast<unsigned int>(volume_data[idx]);
                    unsigned int b = static_cast<unsigned int>(volume_data[idx]);
                    imageFile << r << ' ' << g << ' ' << b << '\n';
                }
            }

            imageFile.close();
        }

        // Upload it to the GPU.
    }
    else
    {
        OutputDebugStringA("Unable to open file!\n");
    }
#endif

    f32 vertices[] =
    {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    Shader shader("../Source/Shaders/Shader.vs", "../Source/Shaders/Shader.fs");

    const glm::vec3 cam_pos(0.f, 0.f, 3.f);
    const glm::vec3 cam_front(0.f, 0.f, -1.f);
    glm::vec3 cam_up = glm::normalize(glm::vec3(0.f, -1.f, 0.f));

    glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_front, cam_up);
    glm::mat4 projection = glm::perspective(glm::radians(45.f), (f32)width / (f32)height, 0.1f, 100.f);
    glm::mat4 pv = projection * view;
    
    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLsizei stride = 5 * sizeof(f32);

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0));
    GLCall(glEnableVertexAttribArray(0));

    GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(3 * sizeof(f32))));
    GLCall(glEnableVertexAttribArray(1));

    // load the image data
    int tex_width, tex_height, tex_channel_count;
    u8* tex_data = stbi_load("../Resources/plank.jpg", &tex_width, &tex_height, &tex_channel_count, 0);
    if (!tex_data)
    {
        OutputDebugStringA("Unable to load texture!\n");
        exit(1);
    }

    GLuint texture;
    GLCall(glGenTextures(1, &texture));

    GLCall(glBindTexture(GL_TEXTURE_2D, texture));

    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data));

    GLCall(glBindTexture(GL_TEXTURE_2D, 0));

    shader.Use();
    GLCall(glUniform1i(glGetUniformLocation(shader.id, "texture_sampler"), 0));

    while (!Win32WindowShouldQuit())
    {
        GLCall(glClearColor(0.1f, 0.1f, 0.1f, 1.f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        shader.Use();
        glm::mat4 pvm = pv * model;
        GLCall(GLint pvm_location = glGetUniformLocation(shader.id, "pvm"));
        GLCall(glUniformMatrix4fv(pvm_location, 1, GL_FALSE, glm::value_ptr(pvm)));

        GLCall(glBindVertexArray(vao));
        GLCall(glBindTexture(GL_TEXTURE_2D, texture));
        GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));

        SwapBuffers(device_context);
    }

    // NOTE(achal): Windows will automatically delete the context when it exits out of the program.

    return 0;
}