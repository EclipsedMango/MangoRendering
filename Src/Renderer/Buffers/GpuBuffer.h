#ifndef MANGORENDERING_GPUBUFFER_H
#define MANGORENDERING_GPUBUFFER_H

#include <cstdint>
#include <cstddef>

class GpuBuffer {
public:
    explicit GpuBuffer(size_t size, uint32_t binding);
    virtual ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    virtual void SetData(const void* data, size_t size, size_t offset) const;
    virtual void Bind() const = 0;

    [[nodiscard]] size_t GetSize() const { return m_Size; }
    [[nodiscard]] uint32_t GetId() const { return m_ID; }

protected:
    unsigned int m_ID = 0;
    size_t m_Size;
    uint32_t m_Binding;
};


#endif //MANGORENDERING_GPUBUFFER_H