
#ifndef MANGORENDERING_SHADERSTORAGEBUFFER_H
#define MANGORENDERING_SHADERSTORAGEBUFFER_H

#include <cstdint>
#include <cstddef>

class ShaderStorageBuffer {
public:
    // size in bytes, binding is the layout(binding = N) in shader
    ShaderStorageBuffer(size_t size, uint32_t binding);
    ~ShaderStorageBuffer();

    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

    void SetData(const void* data, size_t size, size_t offset = 0) const;
    void Bind() const;

    [[nodiscard]] size_t GetSize() const { return m_Size; }
    [[nodiscard]] uint32_t GetId() const { return m_ID; }

private:
    unsigned int m_ID = 0;
    uint32_t m_Binding;
    size_t m_Size;
};


#endif //MANGORENDERING_SHADERSTORAGEBUFFER_H