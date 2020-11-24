#include "Core/Win32.h"
#include "Util.h"
#include "ArcballCamera.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Window.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <filesystem>
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


bool show_demo_window = true;
bool show_settings_window = false;
bool show_about_window = false;
bool show_file_browser = false;
bool show_hidden_items = false;
std::string current_dir = "../";
ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick
    | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

float sampling_rate = 2.f;

void ListFilesAndDirectories()
{
    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    for (const auto& entry : std::filesystem::directory_iterator(current_dir))
    {
        const std::filesystem::path& filepath = entry.path();
        std::string extension = filepath.extension().string();

        bool is_item_hidden = FILE_ATTRIBUTE_HIDDEN & GetFileAttributesA(filepath.string().c_str());
        bool is_item_displayable = (!is_item_hidden || (is_item_hidden && show_hidden_items))
            && (entry.is_directory() || extension == ".raw" || extension == ".pvm");
        if (is_item_displayable)
        {
            const std::string& path = filepath.filename().string();

            // Todo: Display directory names in other color (say yellow) than the file names, and display
            // hidden items in other colors as well
            if (ImGui::TreeNodeEx(path.c_str(), flags))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                {
                    if (entry.is_directory())
                    {
                        current_dir.append("/" + path);
                    }
                    else
                    {
                        // Todo: Open the damn volume
                    }
                }
            }
        }
    }
}

void ListLogicalDrives()
{
    char buffer[64];
    DWORD length = GetLogicalDriveStringsA(64, buffer);

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());

    for (unsigned long i = 0; i < length; ++i)
    {
        bool is_drive_name = buffer[i] != ':' && buffer[i] != '\\' && buffer[i] != '\0';
        if (is_drive_name)
        {
            if (ImGui::TreeNodeEx(&buffer[i], flags))
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered(ImGuiHoveredFlags_None))
                {
                    current_dir += buffer[i];
                    current_dir += ":/";
                }
            }
        }
    }
}

struct Application
{
    Application(Window* window)
    {
        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

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

        // Note: You can remove imgui_demo.cpp from the project if you don't need this ImGui::ShowDemoWindow call
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "CTRL + O"))
                {
                    show_file_browser = true;
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

        if (show_file_browser)
        {
            ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            ImVec2 file_browser_dimensions = { 0.5f * ImGui::GetIO().DisplaySize.x, 0.5f * ImGui::GetIO().DisplaySize.y };
            ImGui::SetNextWindowSizeConstraints(file_browser_dimensions, file_browser_dimensions);

            ImGui::OpenPopup("File Browser");
            if (ImGui::BeginPopupModal("File Browser", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
            {
                // Distribute the file browser dimensions (in ratios) for different parts of the file browser
                // 10% for the top bar showing the current directory and has the up button
                // 62% for the enclosure which lists all the files
                // 10% for the text field
                // 10% for the Show Hidden Items checkbox and "Open" and "Cancel" buttons

                std::string absolute_filepath = std::filesystem::absolute(current_dir).string();

                // Todo: I need to make buttons for different parts of the filepath, instead of just a
                // simple text string.
                
                const float file_tab_height = 0.1f * file_browser_dimensions.y;
                ImGui::BeginChild("FileTab", ImVec2(0, file_tab_height));
                if (ImGui::ArrowButton("ButtonUp", ImGuiDir_Up))
                {
                    // You cannot go back (up) a directory if you're already in the root directory of a logical
                    // drive. From there just empty out the current directory and start building it again once
                    // the user selects the logical drive.
                    bool cannot_go_up = absolute_filepath.empty() || absolute_filepath.length() == 3;
                    if (cannot_go_up)
                    {
                        current_dir = "";
                    }
                    else
                    {
                        current_dir += "/..";
                        absolute_filepath = std::filesystem::absolute(current_dir).string();
                    }
                }

                // Computer => C: => Projects => VolumeRenderer
                ImGui::SameLine();
                if (ImGui::Button("Computer"))
                {

                }

                ImGui::SameLine();
                ImGui::ArrowButtonEx("FakeButton", ImGuiDir_Right, ImVec2(10, 10), ImGuiButtonFlags_Disabled);

                ImGui::SameLine();
                if (ImGui::Button("C:"))
                {
                }

                ImGui::SameLine();
                if (ImGui::Button("Projects"))
                {

                }

                ImGui::SameLine();
                if (ImGui::Button("VolumeRenderer"))
                {

                }


#if 0
                ImGui::SameLine();
                ImGui::Text(absolute_filepath.c_str());

                ImGui::SameLine();
                ImGui::Text("Computer");
#endif
                ImGui::EndChild();

                const float file_list_height = 0.62f * file_browser_dimensions.y;
                ImGui::BeginChild("FileList", ImVec2(0, file_list_height), true);
                if (!current_dir.empty())
                {
                    ListFilesAndDirectories();
                }
                else
                {
                    ListLogicalDrives();
                }
                ImGui::EndChild();

                const float search_field_height = 0.1f * file_browser_dimensions.y;
                ImGui::BeginChild("SearchField", ImVec2(0, search_field_height), false);
                char buf[64];
                ImGui::InputText("Label", buf, 64);
                ImGui::EndChild();

                const float button_group_height = 0.1f * file_browser_dimensions.y;
                ImGui::BeginChild("ButtonGroup", ImVec2(0, button_group_height));
                ImGui::SetCursorPosY(button_group_height * 0.2f);

                ImGui::Checkbox("Show Hidden Files", &show_hidden_items);

                ImVec2 button_size = { button_group_height * 2.f, button_group_height * 0.75f };
                ImGui::SameLine();
                ImGui::SetCursorPosX(file_browser_dimensions.x - 2.25f * button_size.x);

                // Open Button
                if (ImGui::Button("Open", button_size))
                {

                }

                ImGui::SameLine();

                // Cancel Button
                if (ImGui::Button("Cancel", button_size))
                {
                    show_file_browser = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndChild();

                ImGui::EndPopup();
            }
        }

        ImGui::Render();
    }

    void EndFrame() const
    {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    int width = 1280;
    int height = 720;
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

    glm::vec3 spacing(1.f);
    glm::ivec3 volume_dims(256, 256, 256);
    std::ifstream file("../Resources/skull_256x256x256_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);

    // glm::vec3 spacing(1.f, 1.f, 4.f);
    // glm::ivec3 volume_dims(341, 341, 93);
    // std::ifstream file("../Resources/statue_leg_341x341x93_uint8.raw", std::ios::in | std::ios::binary | std::ios::ate);

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
        shader.SetUniform1f("sampling_rate", sampling_rate);

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