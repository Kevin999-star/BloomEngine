#pragma once
#include <glm/glm.hpp>

enum class GizmoMode
{
    Translate,
    Rotate,
    Scale
};

class Gizmos
{
public:
    static void Draw(const glm::mat4& view, const glm::mat4& proj, glm::vec3& position);
    static GizmoMode s_Mode;
};
