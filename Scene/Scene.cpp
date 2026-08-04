#include "Scene.h"
#include <glm/glm.hpp>


// === FUNCIÓN GLOBAL DE COLISIÓN ===
bool CheckCollision(const Entity& a, const Entity& b)
{
    glm::vec3 aMin = a.position + a.colliderMin * a.scale;
    glm::vec3 aMax = a.position + a.colliderMax * a.scale;

    glm::vec3 bMin = b.position + b.colliderMin * b.scale;
    glm::vec3 bMax = b.position + b.colliderMax * b.scale;

    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
        (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
        (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

void Scene::AddEntity(const Entity& e)
{
    entities.push_back(e);
}



