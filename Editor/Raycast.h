#pragma once
#include <glm/glm.hpp>

glm::vec3 ScreenToWorldRay(
    float mouseX, float mouseY,
    float viewportWidth, float viewportHeight,
    const glm::mat4& view,
    const glm::mat4& projection
);
