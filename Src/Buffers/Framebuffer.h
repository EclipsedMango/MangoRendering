#ifndef MANGORENDERING_FRAMEBUFFER_H
#define MANGORENDERING_FRAMEBUFFER_H
#include <cstdint>

enum class FramebufferType {
    ColorDepth,  // color + depth, for post-processing etc
    DepthOnly,   // shadow maps
    DepthArray   // depth texture for csm
};

class Framebuffer {
public:
    Framebuffer(uint32_t width, uint32_t height, FramebufferType type, uint32_t layers = 1);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Bind() const;
    static void Unbind();

    // only valid for deptharray
    void BindLayer(uint32_t layer) const;

    void Resize(uint32_t width, uint32_t height);

    [[nodiscard]] uint32_t GetColorAttachment() const { return m_colorAttachment; }
    [[nodiscard]] uint32_t GetDepthAttachment() const { return m_depthAttachment; }
    [[nodiscard]] uint32_t GetWidth() const { return m_width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_height; }

private:
    void Create();
    void Destroy() const;

    uint32_t m_fbo = 0;
    uint32_t m_colorAttachment = 0;
    uint32_t m_depthAttachment = 0;
    uint32_t m_layers = 1;
    uint32_t m_width;
    uint32_t m_height;
    FramebufferType m_type;
};


#endif //MANGORENDERING_FRAMEBUFFER_H