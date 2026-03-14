
#include "GpuBuffer.h"

#include "glad/gl.h"

GpuBuffer::GpuBuffer(const size_t size, const uint32_t binding) : m_Size(size), m_Binding(binding) {
    glCreateBuffers(1, &m_ID);
    glNamedBufferData(m_ID, static_cast<GLsizeiptr>(size), nullptr, GL_DYNAMIC_DRAW);
}

GpuBuffer::~GpuBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void GpuBuffer::SetData(const void* data, const size_t size, const size_t offset) const {
    glNamedBufferSubData(m_ID, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}
