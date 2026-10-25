#pragma once
#include <MME/Assets.h>
#include <glm/glm.hpp>
#include <cstdint>

namespace mme {

    class OrthoCamera;

    struct SpriteDesc {
        TextureID texture = 0;          // 0 = düz renk (beyaz doku)
        glm::vec2 pos{ 0,0 };
        glm::vec2 size{ 1,1 };
        glm::vec4 tint{ 1,1,1,1 };
        float     rotation = 0.0f;      // derece
        glm::vec2 origin{ 0.5f, 0.5f }; // pos'un quad üzerindeki yeri (0..1)
    };

    struct SpriteUVDesc {
        TextureID texture = 0;
        glm::vec2 pos{ 0,0 };
        glm::vec2 size{ 1,1 };
        glm::vec2 uvMin{ 0,0 };
        glm::vec2 uvMax{ 1,1 };
        glm::vec4 tint{ 1,1,1,1 };
        float     rotation = 0.0f;
        glm::vec2 origin{ 0.5f, 0.5f };
    };

    // Izgara şeklindeki bir dokuda (atlas / sprite sheet) index'inci hücrenin UV'leri.
    // index soldan sağa, yukarıdan aşağı sayılır (0 = sol-üst hücre).
    void GridCellUV(int columns, int rows, int index, glm::vec2& uvMin, glm::vec2& uvMax);

    // Batch'li 2D çizici: quad'ları CPU'da biriktirir, 8 texture slotuna kadar
    // tek draw call ile gönderir. Tüm çizimler BeginScene/EndScene arasında olur.
    class Renderer2D {
    public:
        static void Init(int maxQuads = 10000);
        static void Shutdown();

        static void SetViewport(int width, int height);
        static void Clear(const glm::vec4& color);

        static void BeginScene(const glm::mat4& viewProjection);
        static void BeginScene(const OrthoCamera& camera);
        static void EndScene();

        static void DrawQuad(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color);
        // Debug çizimleri: kalınlık ekran pikseli cinsinden (zoom'dan bağımsız)
        static void DrawLine(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color, float thicknessPx = 1.5f);
        static void DrawRectOutline(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color, float thicknessPx = 1.5f);
        static void DrawSprite(const SpriteDesc& s);
        static void DrawSpriteUV(const SpriteUVDesc& s);

        // UI geçişi için: (topLeft) sol-üst köşe, +y aşağı.
        static void DrawScreenRect(const glm::vec2& topLeft, const glm::vec2& size,
                                   const glm::vec4& color, TextureID texture = 0);

        static TextureID GetWhiteTexture();

        struct Stats { uint32_t drawCalls = 0; uint32_t quadCount = 0; uint32_t textureBinds = 0; };
        static void ResetStats();
        static Stats GetStats();
    };
}
