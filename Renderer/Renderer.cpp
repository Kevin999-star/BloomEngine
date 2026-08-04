#include "Renderer.h"
#include <glad/glad.h>

void Renderer::Clear()
{
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // ← 🔥 AÑADIR ESTO
}


void Renderer::Draw(const VertexArray& vao, const IndexBuffer& ibo)
{
    vao.Bind();
    ibo.Bind();
    glDrawElements(GL_TRIANGLES, ibo.GetCount(), GL_UNSIGNED_INT, nullptr);
}
