#include "Camera.h"

Camera::Camera(glm::vec3 position, float yaw, float pitch, float fov, float aspect)
    : Position(position), Yaw(yaw), Pitch(pitch), Fov(fov), Aspect(aspect),
      WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
{
    UpdateCameraVectors();
}

void Camera::ProcessKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right)
{
    float speed = 5.0f * deltaTime;

    if (forward)
        Position += Front * speed;
    if (backward)
        Position -= Front * speed;
    if (left)
        Position -= Right * speed;
    if (right)
        Position += Right * speed;
}

void Camera::ProcessMouse(float xoffset, float yoffset)
{
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw   += xoffset;
    Pitch -= yoffset;

    if (Pitch > 89.0f)  Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(Fov), Aspect, 0.1f, 100.0f);
}

void Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
