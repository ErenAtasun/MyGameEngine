#pragma once
#include <MME/Assets.h>

namespace mme {

    // Ekran dışı çizim hedefi (renk dokusu). Editör sahneyi buna çizer ve dokuyu
    // bir ImGui penceresinde gösterir.
    //
    //   fb.Bind();                         // viewport fb boyutuna ayarlanır
    //   Renderer2D::Clear(...); BeginScene(...); ...; EndScene();
    //   fb.Unbind();                       // önceki hedef ve viewport geri gelir
    //   ImGui::Image(fb.GetColorTexture(), size, {0,1}, {1,0});
    class Framebuffer {
    public:
        Framebuffer(int width, int height);
        ~Framebuffer();
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        void Resize(int width, int height);  // aynı boyutsa bir şey yapmaz
        void Bind();
        void Unbind();

        TextureID GetColorTexture() const { return m_Color; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        void Create();
        void Destroy();

        uint32_t m_FBO = 0;
        TextureID m_Color = 0;
        int m_Width = 0, m_Height = 0;
        int m_PrevViewport[4] = {};
        int m_PrevFBO = 0;
    };
}
