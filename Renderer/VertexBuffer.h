#pragma once
#include <cstdint>

class VertexBuffer
{
public:
    VertexBuffer(const void* data, uint32_t size);
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;

    uint32_t GetID() const { return m_RendererID; }   // ⭐ AÑADE ESTO

private:
    uint32_t m_RendererID;
};
