#pragma once
#include <string>
#include <cstdint>

class Texture
{
public:
    Texture() = default;
    Texture(const std::string& path);
    ~Texture();

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetID() const { return m_RendererID; }   
    // 🚫 PROHIBIR COPIA (esto evita tu crash)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

private:
    uint32_t m_RendererID;
    int m_Width, m_Height, m_Channels;
};
