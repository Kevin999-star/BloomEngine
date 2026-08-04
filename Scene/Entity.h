#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh;
class Material;

struct Entity
{
    glm::vec3 position{ 0,0,0 };
    glm::vec3 rotation{ 0,0,0 };
    glm::vec3 scale{ 1,1,1 };

    Mesh* mesh = nullptr;
    Material* material = nullptr;

    glm::vec3 colliderMin{ 0,0,0 };
    glm::vec3 colliderMax{ 0,0,0 };
    glm::vec3 velocity = glm::vec3(0.0f);

    glm::mat4 GetModelMatrix() const;

    //glm::vec3 velocity = glm::vec3(0.0f);
    float radius = 0.5f;
    bool isBall = false;
    float elasticity = 0.85f;
    float friction = 0.98f;

  
};
