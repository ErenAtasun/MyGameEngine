#include <MME/Renderer2D.h>
#include <MME/Log.h>
#include <MME/OrthoCamera.h>

#include <glad/glad.h>

#include <algorithm>   // std::min
#include <array>
#include <cmath>
#include <cstddef>     // offsetof
#include <vector>

namespace mme {

    namespace {
        // Motorun yerleşik sprite shader'ı: çalışmak için dışarıdan dosya gerekmez.
        const char* kSpriteVS = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aTexIndex;

uniform mat4 uViewProjection;

out vec2 vUV;
out vec4 vColor;
flat out int vTexIndex;

void main() {
    vUV = aUV;
    vColor = aColor;
    vTexIndex = int(aTexIndex + 0.5);
    gl_Position = uViewProjection * vec4(aPos, 0.0, 1.0);
}
)";

        const char* kSpriteFS = R"(#version 330 core
in vec2 vUV;
in vec4 vColor;
flat in int vTexIndex;

out vec4 FragColor;

uniform sampler2D uTextures[8];

// GLSL 330'da sampler dizisi yalnızca sabit indeksle okunabilir
vec4 SampleFromIndex(int i, vec2 uv) {
    if (i == 0) return texture(uTextures[0], uv);
    if (i == 1) return texture(uTextures[1], uv);
    if (i == 2) return texture(uTextures[2], uv);
    if (i == 3) return texture(uTextures[3], uv);
    if (i == 4) return texture(uTextures[4], uv);
    if (i == 5) return texture(uTextures[5], uv);
    if (i == 6) return texture(uTextures[6], uv);
    return texture(uTextures[7], uv);
}

void main() {
    FragColor = SampleFromIndex(vTexIndex, vUV) * vColor;
}
)";

        constexpr int kMaxTextureSlots = 8; // shader'daki uTextures boyutu

        struct QuadVertex { glm::vec2 pos; glm::vec2 uv; glm::vec4 color; float texIndex; };

        struct RendererData {
            GLuint vao = 0, vbo = 0, ebo = 0, program = 0;
            GLint viewProjLocation = -1;

            int maxQuads = 0;
            std::vector<QuadVertex> vertices;   // CPU tarafı batch
            uint32_t quadCount = 0;

            std::array<TextureID, kMaxTextureSlots> textureSlots{};
            int textureSlotCount = 1;           // slot 0 = beyaz
            int maxTextureSlots = kMaxTextureSlots;
            TextureID whiteTexture = 0;

            bool inScene = false;
            glm::mat4 viewProjection{ 1.0f };
            int viewportW = 1, viewportH = 1;
            Renderer2D::Stats stats;
        };

        RendererData s_Data;

        void StartBatch() {
            s_Data.quadCount = 0;
            s_Data.textureSlotCount = 1;
        }

        void Flush() {
            if (s_Data.quadCount == 0) return;

            glBindBuffer(GL_ARRAY_BUFFER, s_Data.vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(s_Data.quadCount * 4 * sizeof(QuadVertex)), s_Data.vertices.data());

            for (int i = 0; i < s_Data.textureSlotCount; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, s_Data.textureSlots[(size_t)i]);
                s_Data.stats.textureBinds++;
            }

            glUseProgram(s_Data.program);
            glBindVertexArray(s_Data.vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)(s_Data.quadCount * 6), GL_UNSIGNED_INT, nullptr);
            s_Data.stats.drawCalls++;

            StartBatch();
        }
    }

    void Renderer2D::Init(int maxQuads)
    {
        s_Data.maxQuads = maxQuads;
        s_Data.vertices.resize((size_t)maxQuads * 4);

        GLint units = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
        s_Data.maxTextureSlots = std::min(kMaxTextureSlots, (int)units);

        glGenVertexArrays(1, &s_Data.vao);
        glBindVertexArray(s_Data.vao);

        glGenBuffers(1, &s_Data.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_Data.vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(s_Data.vertices.size() * sizeof(QuadVertex)), nullptr, GL_DYNAMIC_DRAW);

        // İndeks buffer'ı hiç değişmez: quad başına 0-1-2, 2-3-0
        std::vector<uint32_t> indices((size_t)maxQuads * 6);
        for (uint32_t q = 0, v = 0; q < (uint32_t)maxQuads; ++q, v += 4) {
            uint32_t* i = &indices[(size_t)q * 6];
            i[0] = v + 0; i[1] = v + 1; i[2] = v + 2;
            i[3] = v + 2; i[4] = v + 3; i[5] = v + 0;
        }
        glGenBuffers(1, &s_Data.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, uv));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, texIndex));

        s_Data.program = Assets::CreateShaderProgram(kSpriteVS, kSpriteFS, "Renderer2D/sprite");
        MME_ASSERT(s_Data.program != 0, "Yerlesik sprite shader'i derlenemedi");
        s_Data.viewProjLocation = glGetUniformLocation(s_Data.program, "uViewProjection");
        glUseProgram(s_Data.program);
        const int samplers[kMaxTextureSlots] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        glUniform1iv(glGetUniformLocation(s_Data.program, "uTextures"), kMaxTextureSlots, samplers);

        const uint32_t white = 0xFFFFFFFFu;
        s_Data.whiteTexture = Assets::CreateTexture(1, 1, &white, TextureFilter::Nearest);
        s_Data.textureSlots[0] = s_Data.whiteTexture;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        StartBatch();
    }

    void Renderer2D::Shutdown()
    {
        Assets::DestroyTexture(s_Data.whiteTexture);
        if (s_Data.program) glDeleteProgram(s_Data.program);
        if (s_Data.ebo) glDeleteBuffers(1, &s_Data.ebo);
        if (s_Data.vbo) glDeleteBuffers(1, &s_Data.vbo);
        if (s_Data.vao) glDeleteVertexArrays(1, &s_Data.vao);
        s_Data = RendererData{};
    }

    void GridCellUV(int columns, int rows, int index, glm::vec2& uvMin, glm::vec2& uvMax) {
        columns = std::max(columns, 1);
        rows = std::max(rows, 1);
        const int col = index % columns;
        const int row = index / columns; // yukarıdan
        // Dokularda v=0 görüntünün altı: üstten row. satır v'de ters
        uvMin = { (float)col / columns,       1.0f - (float)(row + 1) / rows };
        uvMax = { (float)(col + 1) / columns, 1.0f - (float)row / rows };
    }

    void Renderer2D::SetViewport(int width, int height) {
        glViewport(0, 0, width, height);
        s_Data.viewportW = std::max(width, 1);
        s_Data.viewportH = std::max(height, 1);
    }

    void Renderer2D::Clear(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Renderer2D::BeginScene(const glm::mat4& viewProjection)
    {
        MME_ASSERT(!s_Data.inScene, "BeginScene iki kez cagrildi (EndScene eksik)");
        s_Data.inScene = true;
        s_Data.viewProjection = viewProjection;
        glUseProgram(s_Data.program);
        glUniformMatrix4fv(s_Data.viewProjLocation, 1, GL_FALSE, &viewProjection[0][0]);
        StartBatch();
    }

    void Renderer2D::BeginScene(const OrthoCamera& camera) { BeginScene(camera.GetViewProjection()); }

    void Renderer2D::EndScene()
    {
        MME_ASSERT(s_Data.inScene, "EndScene, BeginScene olmadan cagrildi");
        Flush();
        s_Data.inScene = false;
#ifndef NDEBUG
        for (GLenum err; (err = glGetError()) != GL_NO_ERROR;)
            MME_ERROR("[Renderer2D] GL hatasi: 0x", std::hex, err);
#endif
    }

    void Renderer2D::DrawQuad(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color)
    {
        SpriteUVDesc d;
        d.pos = center;
        d.size = size;
        d.tint = color;
        DrawSpriteUV(d);
    }

    namespace {
        // Geçerli projeksiyonda bir ekran pikselinin dünya birimindeki karşılığı
        float WorldPerPixel() {
            const float sx = std::abs(s_Data.viewProjection[0][0]);
            return sx > 0.0f ? 2.0f / (sx * (float)s_Data.viewportW) : 1.0f;
        }
    }

    void Renderer2D::DrawLine(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color, float thicknessPx)
    {
        const glm::vec2 d = b - a;
        const float len = glm::length(d);
        if (len <= 0.0f) return;
        SpriteUVDesc s;
        s.pos = (a + b) * 0.5f;
        s.size = { len, thicknessPx * WorldPerPixel() };
        s.rotation = glm::degrees(std::atan2(d.y, d.x));
        s.tint = color;
        DrawSpriteUV(s);
    }

    void Renderer2D::DrawRectOutline(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color, float thicknessPx)
    {
        const float t = thicknessPx * WorldPerPixel();
        const glm::vec2 h = size * 0.5f;
        DrawQuad({ center.x, center.y + h.y }, { size.x + t, t }, color); // üst
        DrawQuad({ center.x, center.y - h.y }, { size.x + t, t }, color); // alt
        DrawQuad({ center.x - h.x, center.y }, { t, size.y + t }, color); // sol
        DrawQuad({ center.x + h.x, center.y }, { t, size.y + t }, color); // sağ
    }

    void Renderer2D::DrawSprite(const SpriteDesc& s)
    {
        SpriteUVDesc d;
        d.texture = s.texture;
        d.pos = s.pos;
        d.size = s.size;
        d.tint = s.tint;
        d.rotation = s.rotation;
        d.origin = s.origin;
        DrawSpriteUV(d); // uvMin/uvMax varsayılanı tüm doku
    }

    void Renderer2D::DrawSpriteUV(const SpriteUVDesc& s)
    {
        MME_ASSERT(s_Data.inScene, "Cizim BeginScene/EndScene arasinda olmali");

        // Kapasite kontrolü slot aramasından ÖNCE olmalı: Flush texture slotlarını
        // sıfırlıyor, sonra yapılırsa quad artık bağlı olmayan bir slotu gösterir.
        if (s_Data.quadCount >= (uint32_t)s_Data.maxQuads) Flush();

        float texIndex = 0.0f; // 0 = beyaz doku
        if (s.texture != 0 && s.texture != s_Data.whiteTexture) {
            int found = -1;
            for (int i = 1; i < s_Data.textureSlotCount; ++i) {
                if (s_Data.textureSlots[(size_t)i] == s.texture) { found = i; break; }
            }
            if (found == -1) {
                if (s_Data.textureSlotCount >= s_Data.maxTextureSlots) Flush();
                found = s_Data.textureSlotCount++;
                s_Data.textureSlots[(size_t)found] = s.texture;
            }
            texIndex = (float)found;
        }

        // Yerel köşe -> origin'e göre kaydır -> döndür -> pos'a taşı
        const glm::vec2 originPx = s.origin * s.size;
        const float rad = glm::radians(s.rotation);
        const float c = std::cos(rad);
        const float sn = std::sin(rad);
        auto toWorld = [&](glm::vec2 local) -> glm::vec2 {
            local -= originPx;
            return s.pos + glm::vec2(c * local.x - sn * local.y, sn * local.x + c * local.y);
        };

        QuadVertex* v = &s_Data.vertices[(size_t)s_Data.quadCount * 4];
        v[0] = { toWorld({ 0.0f,     0.0f     }), s.uvMin,                  s.tint, texIndex };
        v[1] = { toWorld({ s.size.x, 0.0f     }), { s.uvMax.x, s.uvMin.y }, s.tint, texIndex };
        v[2] = { toWorld({ s.size.x, s.size.y }), s.uvMax,                  s.tint, texIndex };
        v[3] = { toWorld({ 0.0f,     s.size.y }), { s.uvMin.x, s.uvMax.y }, s.tint, texIndex };

        s_Data.quadCount++;
        s_Data.stats.quadCount++;
    }

    void Renderer2D::DrawScreenRect(const glm::vec2& topLeft, const glm::vec2& size,
                                    const glm::vec4& color, TextureID texture)
    {
        SpriteUVDesc d;
        d.texture = texture;
        d.pos = topLeft;
        d.size = size;
        d.tint = color;
        d.origin = { 0.0f, 0.0f };
        // UI'da +y aşağı bakıyor; dokular v=0 altta saklandığı için v'yi çevir
        d.uvMin = { 0.0f, 1.0f };
        d.uvMax = { 1.0f, 0.0f };
        DrawSpriteUV(d);
    }

    TextureID Renderer2D::GetWhiteTexture() { return s_Data.whiteTexture; }

    void Renderer2D::ResetStats() { s_Data.stats = Stats{}; }
    Renderer2D::Stats Renderer2D::GetStats() { return s_Data.stats; }
}
