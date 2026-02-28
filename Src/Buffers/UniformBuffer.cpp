
#include "UniformBuffer.h"
#include "glad/gl.h"

UniformBuffer::UniformBuffer(const size_t size, const uint32_t binding) : m_Binding(binding) {
    glGenBuffers(1, &m_ID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_ID);
}

UniformBuffer::~UniformBuffer() {
    glDeleteBuffers(1, &m_ID);
}

void UniformBuffer::SetData(const void* data, const size_t size, const size_t offset) const {
    glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}

void UniformBuffer::Bind() const {
    glBindBufferBase(GL_UNIFORM_BUFFER, m_Binding, m_ID);
}