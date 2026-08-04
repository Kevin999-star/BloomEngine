#include "Time.h"
#include <GLFW/glfw3.h>

float Time::s_DeltaTime = 0.0f;
float Time::s_LastFrameTime = 0.0f;
float Time::s_Time = 0.0f;

void Time::Update()
{
    float current = glfwGetTime();
    s_DeltaTime = current - s_Time;
    s_LastFrameTime = current;
    s_Time = current;
}

float Time::GetDeltaTime()
{
    return s_DeltaTime;
}

float Time::GetTime()
{
  
    return s_Time;

}

