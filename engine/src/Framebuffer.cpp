#include <MME/Framebuffer.h>
#include <MME/Log.h>
#include <MME/Renderer2D.h>

#include <glad/glad.h>

#include <algorithm>

namespace mme {

    Framebuffer::Framebuffer(int width, int height)
        : m_Width(std::max(width, 1)), m_Height(std::max(height, 1)) {
        Create();
    }

    Framebuffer::~Framebuffer() { Destroy(); }

    void Framebuffer::Create() {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_Color = tex;

        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            MME_ERROR("[Framebuffer] Eksik framebuffer (", m_Width, "x", m_Height, ")");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_FBO = fbo;
    }

    void Framebuffer::Destroy() {
        if (m_FBO) { GLuint f = m_FBO; glDeleteFramebuffers(1, &f); m_FBO = 0; }
        if (m_Color) { GLuint t = m_Color; glDeleteTextures(1, &t); m_Color = 0; }
    }

    void Framebuffer::Resize(int width, int height) {
        width = std::max(width, 1);
        height = std::max(height, 1);
        if (width == m_Width && height == m_Height) return;
        m_Width = width;
        m_Height = height;
        Destroy();
        Create();
    }

    void Framebuffer::Bind() {
        glGetIntegerv(GL_VIEWPORT, m_PrevViewport);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_PrevFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        Renderer2D::SetViewport(m_Width, m_Height);
    }

    void Framebuffer::Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)m_PrevFBO);
        Renderer2D::SetViewport(m_PrevViewport[2], m_PrevViewport[3]);
    }
}
