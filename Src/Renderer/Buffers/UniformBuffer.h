#ifndef MANGORENDERING_UNIFORMBUFFER_H
#define MANGORENDERING_UNIFORMBUFFER_H

#include <cstdint>
#include <cstddef>

#include "GpuBuffer.h"

class UniformBuffer : public GpuBuffer {
public:
    UniformBuffer(size_t size, uint32_t binding);

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void Bind() const override;
};


#endif //MANGORENDERING_UNIFORMBUFFER_H