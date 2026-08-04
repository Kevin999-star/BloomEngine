#pragma once
#include "VertexArray.h"
#include "IndexBuffer.h"
#include <glm/glm.hpp>

class Renderer {
public:
    static void Clear();
    static void Draw(const VertexArray& vao, const IndexBuffer& ibo);
    static void DrawLine(const glm::vec3& a, const glm::vec3& b);
};
