#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera(glm::vec3 position, float yaw, float pitch, float fov, float aspect);

    void ProcessKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right);
    void ProcessMouse(float xoffset, float yoffset);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float Fov;
    float Aspect;
    void UpdateCameraVectors();
private:
    
};
