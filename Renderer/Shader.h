#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>


class Shader
{
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    void SetInt(const std::string& name, int value); 
    void SetMat4(const std::string& name, const glm::mat4& matrix);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetFloat(const std::string& name, float value);
   




private:
    uint32_t m_RendererID;

    std::string LoadFile(const std::string& path);
    uint32_t CompileShader(uint32_t type, const std::string& source);
};
