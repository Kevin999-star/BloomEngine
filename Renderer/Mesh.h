#pragma once
#include <cstdint>
#include <string>

class Shader;
class VertexArray;
class VertexBuffer;
class IndexBuffer;
class Material;

class Mesh
{
public:
    uint32_t vao = 0;
    uint32_t indexCount = 0;

    VertexArray* vaoPtr = nullptr;
    VertexBuffer* vboPtr = nullptr;
    IndexBuffer* iboPtr = nullptr;

    Material* material = nullptr;

    void Draw(Shader& shader);

    bool LoadFromOBJ(const std::string& path);   // ⭐ AÑADIDO
};

