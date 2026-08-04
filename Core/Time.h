#pragma once

class Time
{
public:
    static void Update();
    static float GetDeltaTime();
    static float GetTime();

private:
    static float s_DeltaTime;
    static float s_LastFrameTime;
    static float s_Time;
};
