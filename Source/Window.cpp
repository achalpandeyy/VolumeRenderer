#include "Window.h"
#include "Util.h"
#include "Core/Win32.h"

#include <imgui.h>
#include <iostream>
#include <sstream>

// Todo: DON'T HARDCODE WINDOW WIDTH AND HEIGHT HERE!!!
// Todo: DON'T MAKE THE CAMERA GLOBAL HERE!!!
extern ArcballCamera camera = ArcballCamera(glm::vec3(0.f, 0.f, 7.5f), glm::vec3(0.f), 1280, 720);

Window::Window(int width, int height, const char* title)
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

    // Todo: Should I enable VSync?

    glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);
    glfwSetCursorPosCallback(handle, MouseCallback);
    glfwSetScrollCallback(handle, ScrollCallback);
}

Window::~Window()
{
    glfwDestroyWindow(handle);
    glfwTerminate();
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    GLCall(glViewport(0, 0, width, height));
}

void Window::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE || ImGui::GetIO().WantCaptureMouse)
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

void Window::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!ImGui::GetIO().WantCaptureMouse)
        camera.SetFOV(yoffset);
}

void Window::GLFWErrorCallback(int error, const char* description)
{
    std::ostringstream oss;
    oss << "GLFW Error: [" << error << "] " << description << std::endl;
    OutputDebugStringA(oss.str().c_str());
}