
#include "ShaderStorageBuffer.h"
#include "glad/gl.h"

ShaderStorageBuffer::ShaderStorageBuffer(const size_t size, const uint32_t binding) : GpuBuffer(size, binding) {
    ShaderStorageBuffer::Bind();
}

void ShaderStorageBuffer::Bind() const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
}