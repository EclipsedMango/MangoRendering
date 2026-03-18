
#include "Texture.h"

#include <array>
#include <cstring>

#include "stb_image.h"
#include <stdexcept>

static void GetFormats(const int channels, const std::string& label, GLenum& internalFormat, GLenum& format) {
    switch (channels) {
        case 1: internalFormat = GL_R8;    format = GL_RED;  break;
        case 2: internalFormat = GL_RG8;   format = GL_RG;   break;
        case 3: internalFormat = GL_RGB8;  format = GL_RGB;  break;
        case 4: internalFormat = GL_RGBA8; format = GL_RGBA; break;
        default: throw std::runtime_error("Unsupported channel count in texture: " + label);
    }
}

Texture::Texture(const std::string& path, const bool flipVertically) {
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    GLenum internalFormat, format;
    GetFormats(m_channels, path, internalFormat, format);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    m_target = GL_TEXTURE_2D;
}

Texture::Texture(const unsigned char *data, int width, int height, int channels) {
    m_width = width;
    m_height = height;
    m_channels = channels;

    // flip vertically (GLTF expects top left UV origin, OpenGL expects bottom left)
    std::vector<unsigned char> flipped(width * height * channels);
    const int rowSize = width * channels;
    for (int y = 0; y < height; y++) {
        memcpy(
            flipped.data() + y * rowSize,
            data + (height - 1 - y) * rowSize,
            rowSize
        );
    }

    GLenum internalFormat, format;
    GetFormats(m_channels, "memory texture", internalFormat, format);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, flipped.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    m_target = GL_TEXTURE_2D;
}

// expected face order:
// 0: +X, 1: -X, 2: +Y, 3: -Y, 4: +Z, 5: -Z
Texture::Texture(const std::vector<std::string> &paths) {
    if (paths.size() != 6) {
        throw std::runtime_error("Cubemap requires exactly 6 faces, got " + std::to_string(paths.size()));
    }

    stbi_set_flip_vertically_on_load(false);

    struct FaceData { unsigned char* pixels; int w, h, ch; };
    std::array<FaceData, 6> faces {};

    for (unsigned int i = 0; i < 6; ++i) {
        auto&[pixels, w, h, ch] = faces[i];
        pixels = stbi_load(paths[i].c_str(), &w, &h, &ch, 0);
        if (!pixels) {
            for (unsigned int j = 0; j < i; ++j) {
                stbi_image_free(faces[j].pixels);
            }

            throw std::runtime_error("Failed to load cubemap face: " + paths[i]);
        }

        if (ch != 3 && ch != 4) {
            for (unsigned int j = 0; j <= i; ++j) {
                stbi_image_free(faces[j].pixels);
            }

            throw std::runtime_error("Unsupported channel count in cubemap face: " + paths[i]);
        }
    }

    m_width = faces[0].w; m_height = faces[0].h; m_channels = faces[0].ch;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    for (unsigned int i = 0; i < 6; ++i) {
        auto&[pixels, w, h, ch] = faces[i];
        GLenum internalFormat, format;
        GetFormats(ch, paths[i], internalFormat, format);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    m_target = GL_TEXTURE_CUBE_MAP;
}

Texture::~Texture() {
    glDeleteTextures(1, &m_id);
}

void Texture::Bind(const uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(m_target, m_id);
}

void Texture::Unbind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(m_target, 0);
}