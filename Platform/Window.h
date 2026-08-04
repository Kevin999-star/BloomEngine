#pragma once
#include <string>
#include <GLFW/glfw3.h>
#define NOMINMAX
#include <Windows.h>


class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool ShouldClose() const;
    void PollEvents();
    void SwapBuffers();

    GLFWwindow* GetNativeWindow() const { return handle; }   // ← NECESARIO

private:
    GLFWwindow* handle;
};
