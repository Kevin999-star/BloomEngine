#pragma once
#include <glm/glm.hpp>

class Shader;
class Texture;

class Material
{
public:
    glm::vec3 albedo = glm::vec3(1.0f);
    Texture*  albedoMap = nullptr;
    Texture*  normalMap = nullptr;

    void Bind(Shader& shader);
};
