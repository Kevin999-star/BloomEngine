#pragma once
#include <GLFW/glfw3.h>

#define KEY_W GLFW_KEY_W
#define KEY_S GLFW_KEY_S
#define KEY_A GLFW_KEY_A
#define KEY_D GLFW_KEY_D


class Input
{
public:
    static void Init(GLFWwindow* window);
    static void Update();

    static bool IsKeyPressed(int key);

    static double GetMouseX();
    static double GetMouseY();
    static double GetMouseDeltaX();
    static double GetMouseDeltaY();

private:
    static GLFWwindow* s_Window;

    static double s_LastMouseX;
    static double s_LastMouseY;
    static double s_MouseX;
    static double s_MouseY;
    static double s_MouseDeltaX;
    static double s_MouseDeltaY;

    static bool s_FirstMouse;
};
