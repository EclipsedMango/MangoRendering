
#include "Texture.h"
#include "stb_image.h"
#include <stdexcept>

Texture::Texture(const std::string& path, const bool flipVertically) {
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    GLenum internalFormat, format;
    if (m_channels == 4) {
        internalFormat = GL_RGBA8;
        format = GL_RGBA;
    } else if (m_channels == 3) {
        internalFormat = GL_RGB8;
        format = GL_RGB;
    } else {
        stbi_image_free(data);
        throw std::runtime_error("Unsupported channel count in texture: " + path);
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_id);
}

void Texture::Bind(const uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::Unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}