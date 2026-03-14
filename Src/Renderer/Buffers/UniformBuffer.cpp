
#include "UniformBuffer.h"
#include "glad/gl.h"

UniformBuffer::UniformBuffer(const size_t size, const uint32_t binding) : GpuBuffer(size, binding) {
    UniformBuffer::Bind();
}

void UniformBuffer::Bind() const {
    glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
}
