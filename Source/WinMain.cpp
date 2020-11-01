#include "Core/Win32.h"
#include "Util.h"
#include "ArcballCamera.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Mesh.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

int width = 1280;
int height = 720;

// TODO: Make this a member of non-platform window class, so it can be retrived by the windows ptr
// (data stored on windows side)
ArcballCamera camera(glm::vec3(0.f, 0.f, 7.5f), glm::vec3(0.f), width, height);

int last_x = width / 2;
int last_y = height / 2;

template <typename T>
void Lerp(unsigned int x0, unsigned int x1, T* values)
{
    T y0 = values[x0];
    T y1 = values[x1];

    T m = (y1 - y0) / (float)(x1 - x0);

    for (unsigned int i = x0 + 1; i <= x1 - 1; ++i)
        values[i] = m * (float)(i - x0) + y0;
}

struct Window
{
    // Note: Creating multiple windows would mean multiple initializations of GLFW, which is
    // fine for now since we have only a single window in the entire application. 
    Window(int width, int height, const char* title)
    {
        glfwSetErrorCallback(GLFWErrorCallback);
        if (!glfwInit())
            exit(-1);

        handle = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!handle)
        {
            glfwTerminate();
            exit(-1);
        }

        glfwMakeContextCurrent(handle);

        // TODO: Should I enable VSync?

        glfwSetWindowUserPointer(handle, this);

        glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);
        glfwSetCursorPosCallback(handle, MouseCallback);
        glfwSetScrollCallback(handle, ScrollCallback);
    }

    ~Window()
    {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }

    inline bool ShouldClose() const
    {
        return glfwWindowShouldClose(handle);
    }

    inline void PollEvents() const
    {
        glfwPollEvents();
    }

    inline void SwapBuffers() const
    {
        glfwSwapBuffers(handle);
    }

    GLFWwindow* handle;
    ImGuiIO* io;

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        GLCall(glViewport(0, 0, width, height));
    }

    static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        Window* this_window = (Window*)glfwGetWindowUserPointer(window);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE || this_window->io->WantCaptureMouse)
        {
            last_x = xpos;
            last_y = ypos;
        }
        else
        {
            if (!(xpos == last_x && ypos == last_y))
            {
                // Update view matrix of the camera to incorporate rotation
                camera.Rotate({ last_x, last_y }, { xpos, ypos });

                last_x = xpos;
                last_y = ypos;
            }
        }
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        camera.SetFOV(yoffset);
    }

    static void GLFWErrorCallback(int error, const char* description)
    {
        std::ostringstream oss;
        oss << "GLFW Error: [" << error << "] " << description << std::endl;
        OutputDebugStringA(oss.str().c_str());
    }
};

struct Application
{
    Application(Window* window)
    {
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        window->io = &ImGui::GetIO();

        ImGui::StyleColorsDark();

        // Load OpenGL
        // TODO: If write my OpenGL application with OpenGL version 4.6, would its .exe not run on machines
        // having an OpenGL version lower than that??
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            exit(-1);
        }

        // TODO: Is it necessary to even set the glsl version??
        ImGui_ImplGlfw_InitForOpenGL(window->handle, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    ~Application()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void BeginFrame() const
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool show_demo_window = true;
        bool show_another_window = false;
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.6f, 1.f);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        // You can remove imgui_demo.cpp from the project if you don't need this ImGui::ShowDemoWindow call
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        ImGui::Render();
    }

    void EndFrame() const
    {
        // Todo: Do I need to call the following line of code only after the rendering has finished??
        // Can't I just call it in BeginFrame.
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    Window window(width, height, "Volume Renderer");

    Application application(&window);
   
    GLCall(glViewport(0, 0, width, height));

    GLuint vol_texture;
    GLCall(glGenTextures(1, &vol_texture));

    GLCall(glBindTexture(GL_TEXTURE_3D, vol_texture));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // glm::vec3 spacing(1.f);
    // glm::ivec3 volume_dims(256, 256, 256);
    // std::ifstream file("../Resources/skull_256x256x256_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);

    glm::vec3 spacing(1.f, 1.f, 4.f);
    glm::ivec3 volume_dims(341, 341, 93);
    std::ifstream file("../Resources/statue_leg_341x341x93_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);

    // glm::vec3 spacing(0.625f, 0.625f, 1.f);
    // glm::ivec3 volume_dims(512, 512, 463);
    // std::ifstream file("../Resources/prone_512x512x463_uint16.raw", std::ios::in | std::ios::binary | std::ios::ate);
    
    if (file.is_open())
    {
        std::streampos pos = file.tellg();
        char* volume_data = new char[pos]; // position of the last character == size of the file
        file.seekg(0, std::ios::beg);
        file.read(volume_data, pos);
        file.close();

        // Upload it to the GPU, assuming that input volume data inherently doesn't have any row alignment.
        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        GLCall(glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, (GLsizei)volume_dims.x, (GLsizei)volume_dims.y, (GLsizei)volume_dims.z,
            0, GL_RED, GL_UNSIGNED_BYTE, volume_data));
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

    Mesh cube(GetUnitCubeVertices(), 3, GetUnitCubeIndices());
    Mesh quad(GetNDCQuadVertices(), 2, GetNDCQuadIndices());

    // Construct Model Matrix (brings to World Space from Model Space)
    spacing *= 0.01f;
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

    shader.Bind();
    shader.SetUniform3i("volume_dims", volume_dims.x, volume_dims.y, volume_dims.z);
    shader.SetUniform1i("volume", 2);
    shader.SetUniform1i("transfer_function", 3);

    Shader entry_exit_shader("../Source/Shaders/EntryExitPoints.vs", "../Source/Shaders/EntryExitPoints.fs");

    Texture2D entry_points(width, height, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);
    Texture2D exit_points(width, height, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);

    GLuint entry_exit_points_fbo;
    GLCall(glGenFramebuffers(1, &entry_exit_points_fbo));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, entry_exit_points_fbo));

    // Attach the attachements
    entry_points.Bind();
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, entry_points.id, 0));

    exit_points.Bind();
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, exit_points.id, 0));

    GLuint depth_rbo;
    GLCall(glGenRenderbuffers(1, &depth_rbo));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo));
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height));
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo));

    {
        // Check Framebuffer status here
        GLCall(bool framebuffer_status = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));
        if (framebuffer_status)
        {
            OutputDebugStringA("NOTE: Framebuffer is complete!\n");
        }
    }

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    while (!window.ShouldClose())
    {
        window.PollEvents();

        application.BeginFrame();

        // Generate Entry and Exit point textures
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, entry_exit_points_fbo));
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glClearColor(0.f, 0.f, 0.f, 1.f));

        entry_exit_shader.Bind();
        glm::mat4 pvm = camera.projection * camera.view * model;
        entry_exit_shader.SetUniformMatrix4fv("pvm", glm::value_ptr(pvm));

        cube.BindVAO();

        // Exit Points
        GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT1));
        GLCall(glDepthFunc(GL_GREATER));

        float old_depth;
        GLCall(glGetFloatv(GL_DEPTH_CLEAR_VALUE, &old_depth));
        GLCall(glClearDepth(0.f));

        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GLCall(glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0));

        // Entry Points
        GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));
        GLCall(glDepthFunc(GL_LESS));
        GLCall(glClearDepth(old_depth));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        GLCall(glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0));

        // Second Pass
        shader.Bind();
        shader.SetUniformMatrix4fv("pvm", glm::value_ptr(pvm));
        shader.SetUniform1i("entry_points_sampler", 0);
        shader.SetUniform1i("exit_points_sampler", 1);

        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GLCall(glClearColor(0.5f, 0.5f, 0.5f, 1.f));
        GLCall(glDisable(GL_DEPTH_TEST));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        quad.BindVAO();

        GLCall(glActiveTexture(GL_TEXTURE0));
        GLCall(glBindTexture(GL_TEXTURE_2D, entry_points.id));
        GLCall(glActiveTexture(GL_TEXTURE1));
        GLCall(glBindTexture(GL_TEXTURE_2D, exit_points.id));

        GLCall(glActiveTexture(GL_TEXTURE2));
        GLCall(glBindTexture(GL_TEXTURE_3D, vol_texture));
        GLCall(glActiveTexture(GL_TEXTURE3));
        GLCall(glBindTexture(GL_TEXTURE_1D, transfer_function_texture));
        
        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));

        application.EndFrame();

        window.SwapBuffers();
    }

    return 0;
}