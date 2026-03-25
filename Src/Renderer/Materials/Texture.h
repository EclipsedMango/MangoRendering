#ifndef MANGORENDERING_TEXTURE_H
#define MANGORENDERING_TEXTURE_H

#include <string>
#include <vector>

#include "Core/PropertyHolder.h"
#include "glad/gl.h"

class Texture : public PropertyHolder {
public:
    // ONLY for internal use and serializing, use other constructors instead
    Texture() = default;

    explicit Texture(const std::string& path, bool flipVertically = true);
    explicit Texture(const unsigned char* data, int width, int height, int channels, std::string key);
    explicit Texture(int width, int height, GLenum internalFormat);
    explicit Texture(int width, int height, GLenum internalFormat, int mipLevels);
    explicit Texture(const std::vector<std::string>& paths);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Bind(uint32_t slot = 0) const;

    void Unbind() const;

    static Texture* LoadHDR(const std::string& path);

    [[nodiscard]] std::string GetPropertyHolderType() const override { return "Texture"; }

    [[nodiscard]] const std::string& GetPath() const { return m_path; }
    [[nodiscard]] GLuint GetGLHandle()  const { return m_id; }
    [[nodiscard]] int GetWidth() const { return m_width; }
    [[nodiscard]] int GetHeight() const { return m_height; }
    [[nodiscard]] int GetChannels() const { return m_channels; }
    [[nodiscard]] bool IsCubemap()  const { return m_target == GL_TEXTURE_CUBE_MAP; }

private:
    void RegisterProperties();

    std::string m_path;
    unsigned int m_id = 0;
    GLenum m_target = GL_TEXTURE_2D;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};


#endif //MANGORENDERING_TEXTURE_H