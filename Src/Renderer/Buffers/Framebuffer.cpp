
#include "Framebuffer.h"

#include <stdexcept>
#include "glad/gl.h"

Framebuffer::Framebuffer(const uint32_t width, const uint32_t height, const FramebufferType type, const uint32_t layers) : m_layers(layers), m_width(width), m_height(height), m_type(type) {
    Create();
}

Framebuffer::~Framebuffer() {
    Destroy();
}

void Framebuffer::Create() {
    glCreateFramebuffers(1, &m_fbo);

    if (m_type == FramebufferType::DepthArray) {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_depthAttachment);
        glTextureStorage3D(m_depthAttachment, 1, GL_DEPTH_COMPONENT32F, m_width, m_height, m_layers);

        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr float border[] = {1,1,1,1};
        glTextureParameterfv(m_depthAttachment, GL_TEXTURE_BORDER_COLOR, border);

        // Don’t attach a specific layer here; we attach the layer per-cascade in BindLayer().
        glNamedFramebufferDrawBuffer(m_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(m_fbo, GL_NONE);
    } else if (m_type == FramebufferType::DepthCubeArray) {
        glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_depthAttachment);

        glTextureStorage3D(m_depthAttachment, 1, GL_DEPTH_COMPONENT32F, m_width, m_height, m_layers);

        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glTextureParameteri(m_depthAttachment, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        // Don’t attach a specific layer here; we attach the layer per-cascade in BindLayer().
        glNamedFramebufferDrawBuffer(m_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(m_fbo, GL_NONE);

    } else if (m_type == FramebufferType::ColorDepth || m_type == FramebufferType::ColorDepthHdr) {
        const GLenum colorFormat = m_type == FramebufferType::ColorDepthHdr ? GL_RGBA16F : GL_RGBA8;
        glCreateTextures(GL_TEXTURE_2D, 1, &m_colorAttachment);
        glTextureStorage2D(m_colorAttachment, 1, colorFormat, m_width, m_height);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_colorAttachment, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_depthAttachment);

        glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH32F_STENCIL8, m_width, m_height);

        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTextureParameteri(m_depthAttachment, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);

        glNamedFramebufferTexture(m_fbo, GL_DEPTH_STENCIL_ATTACHMENT, m_depthAttachment, 0);

    } else if (m_type == FramebufferType::ColorOnly) {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_colorAttachment);
        glTextureStorage2D(m_colorAttachment, 1, GL_RGB16F, m_width, m_height);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_colorAttachment, 0);

    } else if (m_type == FramebufferType::DepthOnly) {
        // depth texture (sampled in shadow shader)
        glCreateTextures(GL_TEXTURE_2D, 1, &m_depthAttachment);
        glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH_COMPONENT32F, m_width, m_height);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(m_depthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr float border[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTextureParameterfv(m_depthAttachment, GL_TEXTURE_BORDER_COLOR, border);

        glNamedFramebufferTexture(m_fbo, GL_DEPTH_ATTACHMENT, m_depthAttachment, 0);
        // no color output
        glNamedFramebufferDrawBuffer(m_fbo, GL_NONE);
        glNamedFramebufferReadBuffer(m_fbo, GL_NONE);
    }

    // skip framebuffer completeness for deptharray
    if (m_type != FramebufferType::DepthArray && m_type != FramebufferType::DepthCubeArray) {
        if (glCheckNamedFramebufferStatus(m_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Framebuffer is not complete");
        }
    }
}

void Framebuffer::Destroy() const {
    if (m_colorAttachment) glDeleteTextures(1, &m_colorAttachment);
    if (m_depthAttachment) glDeleteTextures(1, &m_depthAttachment);
    glDeleteFramebuffers(1, &m_fbo);
}

void Framebuffer::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::BindLayer(const uint32_t layer) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    // attach the requested layer
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthAttachment, 0, static_cast<GLint>(layer));
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // validate per layer (good for debug)
    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    //     throw std::runtime_error("DepthArray framebuffer incomplete for layer");
}

// TODO: this is fine for now but really should be changed to actually resize.
void Framebuffer::Resize(const uint32_t width, const uint32_t height) {
    m_width = width;
    m_height = height;
    Destroy();
    Create();
}
