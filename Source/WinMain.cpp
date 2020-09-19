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

int width = 1280;
int height = 720;

struct ArcballCamera
{
    ArcballCamera(const glm::vec3& center, const glm::vec3& pos, f32 sensitivity = 2.5f, f32 fov = PI_32 / 4.f)
        : arcball_center(center), position(pos), sensitivity(sensitivity), fov(fov)
    {
        glm::vec3 cam_z = glm::normalize(position - center);
        glm::vec3 cam_x = glm::normalize(glm::cross(cam_z, glm::vec3(0.f, 1.f, 0.f)));
        glm::vec3 cam_y = glm::cross(cam_x, cam_z);

        arcball_rotation = glm::mat4(glm::vec4(cam_x, 0.f), glm::vec4(cam_y, 0.f), glm::vec4(cam_z, 0.f), glm::vec4(0.f, 0.f, 0.f, 1.f));
        view = GetArcballRotationToWorld();
    }

    void Rotate(const glm::vec3& prev, const glm::vec3& curr)
    {
        f32 angle = glm::acos(glm::min(1.f, glm::dot(prev, curr)));
        glm::vec3 axis = glm::cross(prev, curr);

        Update(angle, axis);
    }

    glm::vec3 position;
    f32 sensitivity;
    glm::mat4 view;
    f32 fov;

private:
    void Update(f32 angle, const glm::vec3& axis)
    {
        arcball_rotation = glm::rotate(glm::mat4(1.f), sensitivity * angle, axis) * arcball_rotation;
        view = GetArcballRotationToWorld();

        glm::mat4 view_inverse = glm::inverse(view);

        position = glm::vec3(view_inverse[3][0], view_inverse[3][1], view_inverse[3][2]);
    }

    inline glm::mat4 GetArcballRotationToWorld()
    {
        glm::mat4 center_translation = glm::translate(glm::mat4(1.f), -arcball_center);

        f32 view_distance = glm::distance(arcball_center, position);
        glm::mat4 position_translation = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -view_distance));

        return position_translation * arcball_rotation * center_translation;
    }

    const glm::vec3 arcball_center;
    glm::mat4 arcball_rotation;
};

// TODO: Make this a member of non-platform window class, so it can be retrived by the windows ptr
// (data stored on windows side)
ArcballCamera camera(glm::vec3(0.5f), glm::vec3(0.5f, 0.5f, 3.f));

// TODO: This comes together to form a Win32Mouse
int last_x = width / 2;
int last_y = height / 2;
int wheel = 0;

glm::vec3 ScreenToArcball(int x, int y)
{
    // Screen to NDC
    glm::vec3 p = glm::vec3(2.f * ((f32)x / (f32)width) - 1.f, 1.f - 2.f * ((f32)y / (f32)height), 0.f);

    f32 distance_sq = glm::dot(p, p);
    if (distance_sq <= 1.f)
    {
        return glm::vec3(p[0], p[1], glm::sqrt(1.f - distance_sq));
    }
    else
    {
        return glm::normalize(p);
    }
}

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

                glm::vec3 prev = ScreenToArcball(last_x, last_y);
                glm::vec3 curr = ScreenToArcball(mouse_pos.x, mouse_pos.y);

                camera.Rotate(prev, curr);

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
            wheel += GET_WHEEL_DELTA_WPARAM(w_param);

            if (wheel >= WHEEL_DELTA || wheel <= -WHEEL_DELTA)
            {
                // TODO: Put this in camera class.
                f32 strength = 0.05f;
                camera.fov -= strength * (wheel / WHEEL_DELTA);
                camera.fov = glm::clamp(camera.fov, PI_32 / 180.f, PI_32 / 2.f);

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

    GLCall(glEnable(GL_DEPTH_TEST));
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

    f32 vertices[] =
    {
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 1.f, 0.f,

        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 1.f,
        0.f, 1.f, 1.f
    };

    u32 indices[] =
    {
        0, 2, 1,
        0, 3, 2,

        6, 2, 3,
        6, 3, 7,

        1, 2, 6,
        1, 6, 5,

        7, 3, 0,
        7, 0, 4,

        0, 1, 5,
        0, 5, 4,

        6, 7, 4,
        6, 4, 5
    };

    Shader shader("../Source/Shaders/Shader.vs", "../Source/Shaders/Shader.fs");
    
    GLuint vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    GLuint vbo;
    GLCall(glGenBuffers(1, &vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GLsizei stride = 3 * sizeof(f32);

    GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0));
    GLCall(glEnableVertexAttribArray(0));

    GLCall(glBindVertexArray(0));

    GLuint ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    shader.Use();

    // volume_dims
    GLCall(GLint location = glGetUniformLocation(shader.id, "volume_dims"));
    GLCall(glUniform3i(location, 256, 256, 256));

    // volume
    GLCall(location = glGetUniformLocation(shader.id, "volume"));
    GLCall(glUniform1i(location, 0));

    while (!Win32WindowShouldQuit())
    {
        GLCall(glClearColor(0.1f, 0.1f, 0.1f, 1.f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        shader.Use();
        GLCall(location = glGetUniformLocation(shader.id, "cam_pos"));
        GLCall(glUniform3f(location, camera.position.x, camera.position.y, camera.position.z));

        glm::mat4 projection = glm::perspective(camera.fov, (f32)width / (f32)height, 0.1f, 100.f);       

        glm::mat4 pv = projection * camera.view;
        GLCall(GLint pv_location = glGetUniformLocation(shader.id, "pv"));
        GLCall(glUniformMatrix4fv(pv_location, 1, GL_FALSE, glm::value_ptr(pv)));

        GLCall(glBindVertexArray(vao));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
        GLCall(glBindTexture(GL_TEXTURE_3D, vol_texture));
        GLCall(glDrawElements(GL_TRIANGLES, 6 * 2 * 3, GL_UNSIGNED_INT, 0));

        SwapBuffers(device_context);
    }

    // NOTE(achal): Windows will automatically delete the context when it exits out of the program.

    return 0;
}