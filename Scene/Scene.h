#pragma once
#include <vector>
#include "Entity.h"



bool CheckCollision(const Entity& a, const Entity& b);

class Scene
{
public:
    std::vector<Entity> entities;

    void Scene::AddEntity(const Entity& e);
    

    // ⭐ DUPLICAR ENTIDAD ⭐
    Entity& DuplicateEntity(const Entity& original)
    {
        entities.push_back(original);      // copia completa
        return entities.back();            // devuelve la nueva entidad
    }
};


