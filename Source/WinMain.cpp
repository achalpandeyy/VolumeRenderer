#include "Core/Win32.h"
#include "Util.h"
#include "ArcballCamera.h"
#include "Shader.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Window.h"
#include "Volume.h"

#include <stb_image/stb_image_write.h>
#include <imgui.h>
#include <imgui_internal.h>
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
#include <filesystem>

/*
* Todo:
*   ->  There seems to be a CPU memory leak, not sure where
* 
*   ->  I must be leaking some memory by not deleting entry and exit points Texture2D, look into that
* 
*   ->  It would be nice to use std::exception for and get rid of all exit(1)'s in the code, it would give me more
*       information about the error just occured
*   
*   ->  Logging to either VS Output window or a debug console powered by ImGui
* 
*   ->  Currently I can load only uint8 and uint16, add support for more
* 
*   ->  Loading of volume datasets aren't fool-proof, someone could easily mess up loading their data and get run-time
*       error, maybe handle that case and show an error dialog
* 
*   ->  I think we no longer need stb_image.h because we're no longer loading images
*/

// Todo: Don't want my Setting window to be collapseable
bool show_demo_window = false;
bool show_settings_window = false;
bool show_about_window = false;
bool show_file_browser = false;

float sampling_rate = 2.f;

template <typename T>
void Lerp(unsigned int x0, unsigned int x1, T* values)
{
    T y0 = values[x0];
    T y1 = values[x1];

    T m = (y1 - y0) / (float)(x1 - x0);

    for (unsigned int i = x0 + 1; i <= x1 - 1; ++i)
        values[i] = m * (float)(i - x0) + y0;
}


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

std::vector<std::string> ParsePath(std::string path)
{
    // Add a trailing slash
    if (path.back() != '\\')
        path += "\\";

    std::vector<std::string> result;
    std::size_t prev_pos = 0;
    std::size_t pos = path.find("\\", prev_pos);

    while (pos != std::string::npos)
    {
        result.push_back(path.substr(prev_pos, pos - prev_pos));
        prev_pos = pos + 1;
        pos = path.find("\\", prev_pos);
    }
    return result;
}

void DrawFakeArrowButton()
{
    ImGui::SameLine(0.f, 0.f);
    float frame_height = ImGui::GetFrameHeight();
    ImGui::ArrowButtonEx("FakeButton", ImGuiDir_Right, ImVec2(frame_height, frame_height), ImGuiButtonFlags_Disabled);
}

ImVec4 GetListItemTextColor(bool is_directory, bool is_hidden)
{
    if (is_directory && is_hidden)
    {
        // Hidden directory
        return { 0.882f, 0.745f, 0.078f, 0.5f };
    }
    else if (is_directory && !is_hidden)
    {
        // Visible directory
        return { 0.882f, 0.745f, 0.078f, 1.0f };
    }
    else if (!is_directory && is_hidden)
    {
        // Hidden file
        return { 1.f, 1.f, 1.f, 0.5f };
    }
    else
    {
        // Visible file
        return { 1.f, 1.f, 1.f, 1.f };
    }
}

// Todo: Why not use unsigned char for "data"?
GLuint ReadVolumeRAW(const char* path, const glm::ivec3& dimensions, const glm::vec3& spacing, unsigned int byte_count, char* data)
{
    // Read in the volume data
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        // Get size of the file
        file.seekg(0, file.end);
        size_t size = file.tellg();
        file.seekg(0, file.beg);

        data = new char[size];
        file.read(data, size);

        file.close();

        assert(size == (size_t)dimensions.x * dimensions.y * dimensions.z * byte_count);

        // Upload to the GPU
        GLuint volume_texture;
        glGenTextures(1, &volume_texture);
        glBindTexture(GL_TEXTURE_3D, volume_texture);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload it to the GPU, assuming that input volume data inherently doesn't have any row alignment
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (byte_count == 1)
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, dimensions.x, dimensions.y, dimensions.z, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        else
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R16, dimensions.x, dimensions.y, dimensions.z, 0, GL_RED, GL_UNSIGNED_SHORT, data);

        glBindTexture(GL_TEXTURE_3D, 0);

        return volume_texture;
    }
    else
    {
        std::ostringstream oss;
        oss << "Unable to open file at path: " << path << std::endl;
        OutputDebugStringA(oss.str().c_str());

        return (GLuint)-1;
    }
}

// Note: Model Matrix brings to World Space from Model Space
glm::mat4 GetModelMatrix(const glm::ivec3& dimensions, const glm::vec3& spacing)
{
    glm::mat3 basis;
    basis[0] = { dimensions[0] * spacing[0] * 0.01f, 0.f, 0.f };
    basis[1] = { 0.f, dimensions[1] * spacing[1] * 0.01f, 0.f };
    basis[2] = { 0.f, 0.f, dimensions[2] * spacing[2] * 0.01f };

    glm::vec3 offset = -0.5f * (basis[0] + basis[1] + basis[2]);

    glm::mat4 model(1.f);
    model[0] = glm::vec4(basis[0], 0.f);
    model[1] = glm::vec4(basis[1], 0.f);
    model[2] = glm::vec4(basis[2], 0.f);
    model[3] = glm::vec4(offset, 1.f);

    return model;
}

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
    {
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
    }
#endif

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window.handle, true);
    ImGui_ImplOpenGL3_Init();

    bool new_volume = false;

    std::string volume_path = "../Resources/skull_256x256x256_uint8.raw";
    glm::ivec3 volume_dimensions(256);
    glm::vec3 volume_spacing(1.f);
    unsigned int volume_byte_count = 1u;
    char* volume_data = nullptr;

    GLuint volume_texture = ReadVolumeRAW(volume_path.c_str(), volume_dimensions, volume_spacing, volume_byte_count, volume_data);

    glm::mat4 model = GetModelMatrix(volume_dimensions, volume_spacing);
    
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


    Shader shader("../Source/Shaders/Shader.vs", "../Source/Shaders/Shader.fs");

    shader.Bind();
    shader.SetUniform3i("volume_dims", volume_dimensions[0], volume_dimensions[1], volume_dimensions[2]);
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

    bool show_open_file_dialog = false;
    bool show_hidden_items = false;
    bool show_file_details_dialog = false;
    std::string current_dir = "../";
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    bool save_as_png = false;

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
                    show_open_file_dialog = true;
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
            
            if (ImGui::Button("Save as PNG"))
            {
                save_as_png = true;
            }

            ImGui::SliderFloat("Sampling Rate", &sampling_rate, 1.f, 20.f);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (show_open_file_dialog)
        {
            // Set the Open File Dialog to appear at the center
            ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            // Set Open File Dialog's dimensions
            ImVec2 open_file_dialog_dimensions = { 0.5f * ImGui::GetIO().DisplaySize.x, 0.5f * ImGui::GetIO().DisplaySize.y };
            ImGui::SetNextWindowSize(open_file_dialog_dimensions);

            ImGui::OpenPopup("OpenFileDialog");
            if (ImGui::BeginPopupModal("OpenFileDialog", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                std::string absolute_filepath = std::filesystem::absolute(current_dir).string();

                const float open_file_dialog_height = 0.5f * ImGui::GetIO().DisplaySize.y;
                const float file_tab_height = 0.1f * open_file_dialog_height;

                // Draw File Tab
                ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 2.f);
                ImGui::BeginChild("FileTab", ImVec2(0, file_tab_height), true,
                    ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                // Up button
                if (ImGui::ArrowButton("UpButton", ImGuiDir_Up))
                {
                    // You cannot go up a directory if you're already in the root directory of a logical
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

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.f));
                ImGui::SameLine();
                if (ImGui::Button("Computer"))
                {
                    current_dir = "";
                }

                if (!absolute_filepath.empty())
                {
                    std::vector<std::string> directories = ParsePath(absolute_filepath);
                    for (size_t i = 0; i < directories.size(); ++i)
                    {
                        DrawFakeArrowButton();

                        ImGui::SameLine(0.f, 0.f);
                        if (ImGui::Button(directories[i].c_str()))
                        {
                            current_dir = "";
                            for (size_t k = 0; k <= i; ++k)
                            {
                                current_dir += directories[k];
                                current_dir += "\\";
                            }
                        }
                    }
                }
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();

                ImGui::EndChild();

                // Draw File List
                const float file_list_height = 0.72f * open_file_dialog_height;
                ImGui::BeginChild("FileList", ImVec2(0, file_list_height), true);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));

                if (current_dir.empty())
                {
                    // Todo: Visually separate Removable drives
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
                else
                {
                    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
                    for (const auto& entry : std::filesystem::directory_iterator(current_dir))
                    {
                        const std::filesystem::path& filepath = entry.path();
                        std::string extension = filepath.extension().string();

                        bool is_item_hidden = FILE_ATTRIBUTE_HIDDEN & GetFileAttributesW(filepath.wstring().c_str());
                        bool is_item_displayable = (!is_item_hidden || (is_item_hidden && show_hidden_items))
                            && (entry.is_directory() || extension == ".raw" || extension == ".pvm");
                        if (is_item_displayable)
                        {
                            const std::string& path = filepath.filename().string();

                            ImVec4 text_color = GetListItemTextColor(entry.is_directory(), is_item_hidden);
                            ImGui::PushStyleColor(ImGuiCol_Text, text_color);

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
                                        show_file_details_dialog = true;
                                        volume_path = std::filesystem::absolute(filepath).string();
                                    }
                                }
                            }
                            ImGui::PopStyleColor();
                        }
                    }
                }

                ImGui::PopStyleVar();
                ImGui::EndChild();

                const float open_file_dialog_width = 0.5f * ImGui::GetIO().DisplaySize.x;
                const float button_group_height = 0.1f * open_file_dialog_height;
                ImGui::BeginChild("ButtonGroup", ImVec2(0, button_group_height));
                ImGui::SetCursorPosY(button_group_height * 0.2f);

                ImGui::Checkbox("Show Hidden Files", &show_hidden_items);

                ImVec2 button_size = { button_group_height * 2.f, button_group_height * 0.75f };
                ImGui::SameLine();
                ImGui::SetCursorPosX(open_file_dialog_width - 2.25f * button_size.x);

                // Cancel Button
                ImGui::SameLine(550.f);
                if (ImGui::Button("Cancel", button_size))
                {
                    show_open_file_dialog = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndChild();

                if (show_file_details_dialog)
                {
                    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    ImGui::SetNextWindowSize(ImVec2(370, 180));

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 12));

                    ImGui::OpenPopup("File Details");
                    if (ImGui::BeginPopupModal("File Details", &show_file_details_dialog, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
                    {
                        const char* datatype = "uint8";

                        ImGui::TextWrapped("Loading <file-name>..\n"
                            "Please provide following details for the dataset.");

                        std::vector<const char*> datatypes = { "unsigned int 8 bit", "unsigned int 16 bit" };
                        static int item_current = 0;

                        ImGui::Combo("Data Type", &item_current, datatypes.data(), datatypes.size());
                        ImGui::InputInt3("Dimensions", &volume_dimensions[0]);
                        ImGui::InputFloat3("Spacing", &volume_spacing[0]);

                        volume_byte_count = item_current + 1;

                        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionWidth() / 2.f) - 78.f);
                        if (ImGui::Button("Open", ImVec2(72, 27)))
                        {
                            // Send the data over to something responsible for loading the volume data
                            new_volume = true;
                            show_file_details_dialog = false;
                            show_open_file_dialog = false;
                        }

                        ImGui::SameLine(0.f);
                        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionWidth() / 2.f) + 6.f);
                        if (ImGui::Button("Cancel", ImVec2(72, 27)))
                        {
                            show_file_details_dialog = false;

                            // Todo: Make sure to set volume_path back according to the previous volume
                            // if the user indeed decides to cancel to loading of the new volume dataset, to avoid bugs
                        }

                        ImGui::PopStyleVar(2);
                        ImGui::EndPopup();
                    }
                }

                ImGui::EndPopup();
            }
        }

        ImGui::Render();

        // Load and upload the volume data if it has changed
        if (new_volume)
        {
            glDeleteTextures(1, &volume_texture);

            volume_texture = ReadVolumeRAW(volume_path.c_str(), volume_dimensions, volume_spacing, volume_byte_count, volume_data);
            model = GetModelMatrix(volume_dimensions, volume_spacing);

            new_volume = false;
        }

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
        glBindTexture(GL_TEXTURE_3D, volume_texture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D, transfer_function_texture);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#endif
        if (save_as_png)
        {
            unsigned char* pixels = (unsigned char*)malloc(1280 * 720 * 3);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

            stbi_flip_vertically_on_write(1);
            stbi_write_png("image.png", width, height, 3, pixels, width * 3);
            free(pixels);

#ifdef _DEBUG
            // Check the default framebuffer data type because it will affect PNG writing
            GLint value = 0;

            glGetFramebufferParameteriv(GL_FRAMEBUFFER, GL_IMPLEMENTATION_COLOR_READ_FORMAT, &value);
            assert(value == GL_RGBA);

            glGetFramebufferParameteriv(GL_FRAMEBUFFER, GL_IMPLEMENTATION_COLOR_READ_TYPE, &value);
            assert(value == GL_UNSIGNED_BYTE);

#endif
            save_as_png = false;
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        window.SwapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}