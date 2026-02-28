
#ifndef MANGORENDERING_SHADERSTORAGEBUFFER_H
#define MANGORENDERING_SHADERSTORAGEBUFFER_H

#include <cstdint>
#include <cstddef>

#include "GpuBuffer.h"

class ShaderStorageBuffer : public GpuBuffer {
public:
    ShaderStorageBuffer(size_t size, uint32_t binding);

    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

    void Bind() const override;
};


#endif //MANGORENDERING_SHADERSTORAGEBUFFER_H