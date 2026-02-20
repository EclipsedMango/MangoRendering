#ifndef MANGORENDERING_UNIFORMBUFFER_H
#define MANGORENDERING_UNIFORMBUFFER_H

#include <cstdint>
#include <cstddef>

class UniformBuffer {
public:
    UniformBuffer(size_t size, uint32_t binding);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void SetData(const void* data, size_t size, size_t offset = 0) const;
    void Bind() const;

private:
    unsigned int m_ID = 0;
    uint32_t m_Binding;
};


#endif //MANGORENDERING_UNIFORMBUFFER_H