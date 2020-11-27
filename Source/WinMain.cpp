#include "Core/Win32.h"
#include "Util.h"
#include "ArcballCamera.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Window.h"
#include "ImGuiFileBrowser.h"
#include "Volume.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

template <typename T>
void Lerp(unsigned int x0, unsigned int x1, T* values)
{
    T y0 = values[x0];
    T y1 = values[x1];

    T m = (y1 - y0) / (float)(x1 - x0);

    for (unsigned int i = x0 + 1; i <= x1 - 1; ++i)
        values[i] = m * (float)(i - x0) + y0;
}

// Todo: Don't want my Setting window to be collapseable
bool show_demo_window = false;
bool show_settings_window = false;
bool show_about_window = false;
bool show_file_browser = false;

float sampling_rate = 2.f;

// Todo: It would be nice to use std::exception for this and all other exit(1)'s in the code
#ifdef _DEBUG
void WINAPI GLDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length,
    const char* message, const void* user_param)
{
    std::ostringstream oss;
    oss << "-----------------------" << std::endl;
    oss << "Debug Message (" << id << "): " << message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: oss << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: oss << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: oss << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: oss << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: oss << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER: oss << "Source: Other"; break;
    }
    oss << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               oss << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: oss << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  oss << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         oss << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         oss << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              oss << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          oss << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           oss << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               oss << "Type: Other"; break;
    }
    oss << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         oss << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       oss << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          oss << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: oss << "Severity: notification"; break;
    }
    oss << std::endl;

    oss << "----------------" << std::endl;

    OutputDebugStringA(oss.str().c_str());

    // Todo: Can you somehow get a line number from OpenGL?
    __debugbreak();
}
#endif

// Todo: Wish I had logging :(
int WINAPI WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev_instance, _In_ LPSTR cmd_line, _In_ int show_code)
{
    int width = 1280;
    int height = 720;
    Window window(width, height, "Volume Renderer");

    // Load OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        OutputDebugStringA("Failed to initialize GLAD\n");
        exit(-1);
    }

#ifdef _DEBUG
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        OutputDebugStringA("Note: Debug context initialized.\n");

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.handle, true);
    ImGui_ImplOpenGL3_Init();

    // Create ImGuiFileBrowser after initializing ImGui, obviously
    ImGuiFileBrowser file_browser;

    // Get the details from ImGuiFileBrowswer and load the volume

    // If the volume is loaded then render it otherwise just display and background color
    // So initially, when the user hasn't selected any .RAW file to load just show the GUI over a default background color.

    Volume vol("../Resources/skull_256x256x256_uint8.raw", glm::ivec3(256), glm::vec3(1.f));
    // Volume vol("../Resources/statue_leg_341x341x93_uint8.raw", glm::ivec3(341, 341, 93), glm::vec3(1.f, 1.f, 4.f));
    // Volume vol("../Resources/prone_512x512x463_uint16.raw", glm::ivec3(512, 512, 463), glm::vec3(0.625f, 0.625f, 1.f));
   
    // Note: You should call glViewport here, GLFW's FramebufferSizeCallback doesn't get called without resizing the window first
    glViewport(0, 0, width, height);

#if 1
    
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
    glGenTextures(1, &transfer_function_texture);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_1D, transfer_function_texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload it to the GPU
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, transfer_function_texel_count, 0, GL_RGBA, GL_FLOAT, &transfer_function[0]);

    glBindTexture(GL_TEXTURE_1D, 0);

    Mesh cube(GetUnitCubeVertices(), 3, GetUnitCubeIndices());
    Mesh quad(GetNDCQuadVertices(), 2, GetNDCQuadIndices());

    // Construct Model Matrix (brings to World Space from Model Space)
    glm::mat3 basis;
    basis[0] = { vol.dimensions.x * vol.spacing.x * 0.01f, 0.f, 0.f };
    basis[1] = { 0.f, vol.dimensions.y * vol.spacing.y * 0.01f, 0.f };
    basis[2] = { 0.f, 0.f, vol.dimensions.z * vol.spacing.z * 0.01f };

    glm::vec3 offset = -0.5f * (basis[0] + basis[1] + basis[2]);

    glm::mat4 model(1.f);
    model[0] = glm::vec4(basis[0], 0.f);
    model[1] = glm::vec4(basis[1], 0.f);
    model[2] = glm::vec4(basis[2], 0.f);
    model[3] = glm::vec4(offset, 1.f);

    Shader shader("../Source/Shaders/Shader.vs", "../Source/Shaders/Shader.fs");

    shader.Bind();
    shader.SetUniform3i("volume_dims", vol.dimensions.x, vol.dimensions.y, vol.dimensions.z);
    shader.SetUniform1i("volume", 2);
    shader.SetUniform1i("transfer_function", 3);

    Shader entry_exit_shader("../Source/Shaders/EntryExitPoints.vs", "../Source/Shaders/EntryExitPoints.fs");

    Texture2D entry_points(width, height, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);
    Texture2D exit_points(width, height, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);

    GLuint entry_exit_points_fbo;
    glGenFramebuffers(1, &entry_exit_points_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, entry_exit_points_fbo);

    // Attach the attachements
    entry_points.Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, entry_points.id, 0);

    exit_points.Bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, exit_points.id, 0);

    GLuint depth_rbo;
    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

    {
        // Check Framebuffer status
        bool framebuffer_status = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        if (framebuffer_status)
        {
            OutputDebugStringA("NOTE: Framebuffer is complete!\n");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    glm::vec4 default_bg(0.5f, 0.5f, 0.5f, 1.f);

    while (!window.ShouldClose())
    {
        window.PollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Note: You can remove imgui_demo.cpp from the project if you don't need this ImGui::ShowDemoWindow call, which you won't
        // need eventually
        if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::GetStyle().WindowRounding = 0.f;
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Todo: This Ctrl + O shortcut doesn't work! 
                if (ImGui::MenuItem("Open", "CTRL + O"))
                {
                    file_browser.SetVisible(true);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings"))
            {
                show_settings_window = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("About"))
            {
                show_about_window = true;
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (show_settings_window)
        {
            ImGui::Begin("Settings");
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::SliderFloat("Sampling Rate", &sampling_rate, 1.f, 20.f);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (file_browser.IsVisible()) file_browser.Open();

        ImGui::Render();

#if 1
        // Generate Entry and Exit point textures
        glBindFramebuffer(GL_FRAMEBUFFER, entry_exit_points_fbo);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.f, 0.f, 0.f, 1.f);

        entry_exit_shader.Bind();
        glm::mat4 pvm = camera.projection * camera.view * model;
        entry_exit_shader.SetUniformMatrix4fv("pvm", glm::value_ptr(pvm));

        cube.BindVAO();

        // Exit Points
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        glDepthFunc(GL_GREATER);

        float old_depth;
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &old_depth);
        glClearDepth(0.f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0);

        // Entry Points
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glDepthFunc(GL_LESS);
        glClearDepth(old_depth);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0);

        // Second Pass
        shader.Bind();
        shader.SetUniformMatrix4fv("pvm", glm::value_ptr(pvm));
        shader.SetUniform1i("entry_points_sampler", 0);
        shader.SetUniform1i("exit_points_sampler", 1);
        shader.SetUniform1f("sampling_rate", sampling_rate);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.5f, 0.5f, 0.5f, 1.f);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        quad.BindVAO();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, entry_points.id);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, exit_points.id);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, vol.texture->id);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D, transfer_function_texture);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#endif

        // glClearColor(default_bg.x, default_bg.y, default_bg.z, default_bg.a);
        // glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.SwapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}