#include "Input.h"

GLFWwindow* Input::s_Window = nullptr;

double Input::s_LastMouseX = 0.0;
double Input::s_LastMouseY = 0.0;
double Input::s_MouseX = 0.0;
double Input::s_MouseY = 0.0;
double Input::s_MouseDeltaX = 0.0;
double Input::s_MouseDeltaY = 0.0;

bool Input::s_FirstMouse = true;

void Input::Init(GLFWwindow* window)
{
    s_Window = window;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Input::Update()
{
    glfwGetCursorPos(s_Window, &s_MouseX, &s_MouseY);

    if (s_FirstMouse)
    {
        s_LastMouseX = s_MouseX;
        s_LastMouseY = s_MouseY;
        s_FirstMouse = false;
    }

    s_MouseDeltaX = s_MouseX - s_LastMouseX;
    s_MouseDeltaY = s_MouseY - s_LastMouseY;

    s_LastMouseX = s_MouseX;
    s_LastMouseY = s_MouseY;
}

bool Input::IsKeyPressed(int key)
{
    return glfwGetKey(s_Window, key) == GLFW_PRESS;
}

double Input::GetMouseX() { return s_MouseX; }
double Input::GetMouseY() { return s_MouseY; }
double Input::GetMouseDeltaX() { return s_MouseDeltaX; }
double Input::GetMouseDeltaY() { return s_MouseDeltaY; }
