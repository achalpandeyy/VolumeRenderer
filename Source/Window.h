#ifndef WINDOW_H

#include "ArcballCamera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

extern ArcballCamera camera;

struct Window
{
    Window(int width, int height, const char* title);
    ~Window();

    inline bool ShouldClose() const { return glfwWindowShouldClose(handle); }
    inline void PollEvents() const { glfwPollEvents(); }
    inline void SwapBuffers() const { glfwSwapBuffers(handle); }

    GLFWwindow* handle;

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void GLFWErrorCallback(int error, const char* description);

    // Todo: Do I really need to make them members of Window?
    // Todo: STATIC???!!
    // Todo: DON'T HARDCODE MAN!!
    // Todo: WHY THE FUCK ARE THESE INLINE???!!!
    inline static int last_x = 1280 / 2;
    inline static int last_y = 720 / 2;
};

#define WINDOW_H
#endif
