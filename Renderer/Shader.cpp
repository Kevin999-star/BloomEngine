#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSrc = LoadFile(vertexPath);
    std::string fragmentSrc = LoadFile(fragmentPath);

    uint32_t vertex = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    uint32_t fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID, vertex);
    glAttachShader(m_RendererID, fragment);
    glLinkProgram(m_RendererID);
    
    int success;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetProgramInfoLog(m_RendererID, 1024, nullptr, infoLog);
        std::cout << "ERROR LINKING PROGRAM:\n" << infoLog << std::endl;
    }


    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

void Shader::Bind() const
{
    std::cout << "Bind() Program ID = " << m_RendererID << std::endl;

    std::cout << "glIsProgram = "
        << glIsProgram(m_RendererID)
        << std::endl;

    glUseProgram(m_RendererID);

    GLenum err = glGetError();
    std::cout << "glUseProgram error = " << err << std::endl;

    GLint current = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    std::cout << "Programa activo = " << current << std::endl;
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

std::string Shader::LoadFile(const std::string& path)
{
    std::cout << "Directorio actual: "
        << std::filesystem::current_path() << std::endl;

    std::cout << "Abriendo: " << path << std::endl;

    std::ifstream file(path);

    if (!file.is_open())
    {
        std::cout << "NO SE PUDO ABRIR\n";
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();

    std::cout << ss.str() << std::endl;

    return ss.str();
}

uint32_t Shader::CompileShader(uint32_t type, const std::string& source)
{
    uint32_t id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];
        glGetShaderInfoLog(id, 1024, nullptr, infoLog);
        std::cout << "ERROR COMPILANDO SHADER ("
            << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
            << "):\n" << infoLog << std::endl;
    }

    return id;
}


void Shader::SetInt(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(m_RendererID, name.c_str()), value);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& matrix)
{
    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value)
{
    GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
    if (loc == -1) return; // evita crash

    glUniform3f(loc, value.x, value.y, value.z);
}


void Shader::SetFloat(const std::string& name, float value)
{
    GLint loc = glGetUniformLocation(m_RendererID, name.c_str());
    if (loc == -1) return; // evita crash

    glUniform1f(loc, value);
}
