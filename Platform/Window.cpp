#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title) {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle)
        throw std::runtime_error("Failed to create window");

    glfwMakeContextCurrent(handle);
    glfwSwapInterval(1);

    glfwSetCursorPosCallback(handle, [](GLFWwindow* window, double xpos, double ypos) {});
    glfwSetKeyCallback(handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {});

}

Window::~Window() {
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle);
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    glfwSwapBuffers(handle);

}