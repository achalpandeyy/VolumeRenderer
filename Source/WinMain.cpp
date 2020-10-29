#include "Core/Win32.h"
#include "Core/Types.h"
#include "ArcballCamera.h"
#include "Shader.h"

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

    T m = (y1 - y0) / (f32)(x1 - x0);

    for (unsigned int i = x0 + 1; i <= x1 - 1; ++i)
        values[i] = m * (f32)(i - x0) + y0;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// TODO: Is is required to free up resources in the destructor?
struct Texture2D
{
    Texture2D(GLsizei width, GLsizei height, GLint internal_format, GLenum format, GLenum type)
    {
        GLCall(glGenTextures(1, &id));
        Bind();

        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        // NOTE: We can assume that these textures doesn't have any row alignment, since we ourselves are creating them
        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, nullptr));
    }

    void inline Bind() const { GLCall(glBindTexture(GL_TEXTURE_2D, id)); }
    void inline Unbind() const { GLCall(glBindTexture(GL_TEXTURE_2D, 0)); }

    GLuint id;
};

struct Mesh
{
    Mesh(const std::vector<float>& vertices, unsigned int vertex_component_count, const std::vector<unsigned int>& indices)
    {
        GLCall(glGenVertexArrays(1, &vao));
        BindVAO();

        GLuint vbo;
        GLCall(glGenBuffers(1, &vbo));
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, vbo));
        GLCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));

        GLsizei stride = vertex_component_count * sizeof(float);
        GLCall(glVertexAttribPointer(0, vertex_component_count, GL_FLOAT, GL_FALSE, stride, (const void*)0));
        GLCall(glEnableVertexAttribArray(0));

        GLuint ibo;
        GLCall(glGenBuffers(1, &ibo));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
        GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));

        UnbindVAO();
    }

    inline void BindVAO() const { GLCall(glBindVertexArray(vao)); }
    inline void UnbindVAO() const { GLCall(glBindVertexArray(0)); }

private:
    GLuint vao;
};

// TODO: Why do we have repeated vertices here??
std::vector<float> GetUnitCubeVertices()
{
    return std::vector<float>
    ({
        0.f, 0.f, 0.f,
        1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 1.f, 0.f,
        0.f, 0.f, 1.f,
        1.f, 0.f, 1.f,
        0.f, 1.f, 1.f,
        1.f, 1.f, 1.f  
    });
}

std::vector<unsigned int> GetUnitCubeIndices()
{
    return std::vector<unsigned int>({ 0, 1, 4, 5, 7, 1, 3, 0, 2, 4, 6, 7, 2, 3 });
}

std::vector<float> GetNDCQuadVertices()
{
    return std::vector<float>
    ({
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
        -1.f, 1.f
    });
}

std::vector<unsigned int> GetNDCQuadIndices()
{
    return std::vector<unsigned int>({ 0, 1, 2, 2, 3, 0 });
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, INT show_code)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(width, height, "Volume Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GLCall(glViewport(0, 0, width, height));

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

        // Upload it to the GPU, assuming that input volume data inherently doesn't have any row alignment.
        GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
        GLCall(glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, 256, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, volume_data));
    }
    else
    {
        // OutputDebugStringA("Unable to open file!\n");
        std::cout << "Unable to open file!" << std::endl;
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

    shader.Bind();
    shader.SetUniform3i("volume_dims", 256, 256, 256);
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
        GLCall(b32 framebuffer_status = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE));
        if (framebuffer_status)
        {
            OutputDebugStringA("NOTE: Framebuffer is complete!\n");
        }
    }

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    while (!glfwWindowShouldClose(window))
    {
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

        f32 old_depth;
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

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    GLCall(glViewport(0, 0, width, height));
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
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

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.SetFOV(yoffset);
}