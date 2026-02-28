
#include "ShaderStorageBuffer.h"
#include "glad/gl.h"

ShaderStorageBuffer::ShaderStorageBuffer(const size_t size, const uint32_t binding) : m_Binding(binding), m_Size(size) {
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);

    // GL_DYNAMIC_DRAW because lights change position/color frequently
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_ID);
}

ShaderStorageBuffer::~ShaderStorageBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void ShaderStorageBuffer::SetData(const void* data, const size_t size, const size_t offset) const {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

void ShaderStorageBuffer::Bind() const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_Binding, m_ID);
}