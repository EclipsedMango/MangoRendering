#ifndef MANGORENDERING_TEXTURE_H
#define MANGORENDERING_TEXTURE_H

#include <string>
#include "glad/gl.h"

class Texture {
public:
    explicit Texture(const std::string& path, bool flipVertically = true);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Bind(uint32_t slot = 0) const;

    static void Unbind();

    [[nodiscard]] int GetWidth() const { return m_width; }
    [[nodiscard]] int GetHeight() const { return m_height; }
    [[nodiscard]] int GetChannels() const { return m_channels; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
};


#endif //MANGORENDERING_TEXTURE_H