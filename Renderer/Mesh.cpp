#include "../External/tinyobjloader/tiny_obj_loader.h"

#include "Mesh.h"
#include "Material.h"
#include "Shader.h"
#include <glad/glad.h>

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include <iostream>

void Mesh::Draw(Shader& shader)
{
    if (material)
        material->Bind(shader);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
}

bool Mesh::LoadFromOBJ(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());

    if (!ok)
    {
        std::cout << "Error cargando OBJ: " << path << std::endl;
        return false;
    }

    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            // Posición del vértice
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];

            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);

            // Índice real
            indices.push_back(indices.size());
        }
    }


    // Crear VAO
    vaoPtr = new VertexArray();
    vaoPtr->Bind();

    // Crear VBO con posiciones
    vboPtr = new VertexBuffer(vertices.data(), vertices.size() * sizeof(float));

    // Crear IBO con índices
    iboPtr = new IndexBuffer(indices.data(), indices.size());

    // Configurar atributo de posición (location = 0)
    vaoPtr->AddVertexBuffer(0, 3, 3 * sizeof(float), (void*)0);

    // Guardar VAO e índice count
    vao = vaoPtr->GetID();
    indexCount = indices.size();

    // Crear material para evitar NULL
    material = new Material();


    return true;
}

