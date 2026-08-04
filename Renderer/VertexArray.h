#pragma once
#include <cstdint>

class VertexArray
{
public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(uint32_t index, int size, int stride, const void* offset);

    uint32_t GetID() const { return m_RendererID; }

private:
    uint32_t m_RendererID;
};
