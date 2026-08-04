#include "Material.h"
#include "Shader.h"
#include "Texture.h"

void Material::Bind(Shader& shader)
{
    // Albedo base
    shader.SetVec3("u_Material.albedo", albedo);

    // Albedo map
    if (albedoMap)
    {
        albedoMap->Bind(0);
        shader.SetInt("u_AlbedoMap", 0);
    }

    // Normal map
    if (normalMap)
    {
        normalMap->Bind(1);
        shader.SetInt("u_NormalMap", 1);
    }
}
