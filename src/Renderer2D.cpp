// src/Renderer2D.cpp
#include "Renderer2D.h"
#include "AssetManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>   // std::min
#include <cstddef>     // offsetof
#include <vector>
#include <iostream>

// ---- Sýnýfýn statik deðiþken TANIMLARI ----
GLuint Renderer2D::s_VAO = 0;
GLuint Renderer2D::s_VBO = 0;
GLuint Renderer2D::s_EBO = 0;
GLuint Renderer2D::s_Program = 0;

Renderer2D::QuadVertex* Renderer2D::s_BufferBase = nullptr;
Renderer2D::QuadVertex* Renderer2D::s_BufferPtr = nullptr;
uint32_t Renderer2D::s_IndexCount = 0;

int Renderer2D::s_MaxSprites = 0;
int Renderer2D::s_MaxVertices = 0;
int Renderer2D::s_MaxIndices = 0;
int Renderer2D::s_MaxTextureSlots = 0;

GLuint* Renderer2D::s_TextureSlots = nullptr;
int Renderer2D::s_TextureSlotCount = 0;
GLuint Renderer2D::s_WhiteTexture = 0;

glm::mat4 Renderer2D::s_Proj(1.0f);
glm::mat4 Renderer2D::s_View(1.0f);

// Ekran koordinatýndan dünya koordinatýna çeviride yükseklik lazým
extern int g_ScreenH; // Config.cpp içinde TANIMLI

void Renderer2D::Init(int maxSprites)
{
    s_MaxSprites = maxSprites;
    s_MaxVertices = s_MaxSprites * 4;
    s_MaxIndices = s_MaxSprites * 6;

    // Maks. texture birimi (frag shader 8 kullanýyor)
    GLint units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
    s_MaxTextureSlots = std::min(8, (int)units);

    // VAO / VBO
    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);

    glGenBuffers(1, &s_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(s_MaxVertices * sizeof(QuadVertex)), nullptr, GL_DYNAMIC_DRAW);

    // Indeks buffer
    std::vector<uint32_t> indices((size_t)s_MaxIndices);
    uint32_t offset = 0;
    for (int i = 0; i < s_MaxSprites; ++i) {
        indices[i * 6 + 0] = offset + 0;
        indices[i * 6 + 1] = offset + 1;
        indices[i * 6 + 2] = offset + 2;
        indices[i * 6 + 3] = offset + 2;
        indices[i * 6 + 4] = offset + 3;
        indices[i * 6 + 5] = offset + 0;
        offset += 4;
    }
    glGenBuffers(1, &s_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    // Vertex layout
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, pos));
    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, uv));
    glEnableVertexAttribArray(2); // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color));
    glEnableVertexAttribArray(3); // texIndex
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, texIndex));

    // Shader (Assets GetShaderFromFiles sampler array'i de ayarlýyor)
    s_Program = Assets::GetShaderFromFiles("shaders/basic.vert", "shaders/sprite.frag");
    glUseProgram(s_Program);

    // 1x1 beyaz texture (slot 0)
    glGenTextures(1, &s_WhiteTexture);
    glBindTexture(GL_TEXTURE_2D, s_WhiteTexture);
    const unsigned int white = 0xFFFFFFFFu;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Texture slot listesi
    s_TextureSlots = new GLuint[s_MaxTextureSlots];
    s_TextureSlots[0] = s_WhiteTexture;   // 0 = white
    s_TextureSlotCount = 1;

    // CPU tarafý vertex buffer
    s_BufferBase = (QuadVertex*)std::malloc((size_t)s_MaxVertices * sizeof(QuadVertex));
    s_BufferPtr = s_BufferBase;

    // Alfa blend
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    StartBatch();
}

void Renderer2D::Shutdown()
{
    std::free(s_BufferBase);     s_BufferBase = nullptr;
    delete[] s_TextureSlots;     s_TextureSlots = nullptr;

    if (s_WhiteTexture) glDeleteTextures(1, &s_WhiteTexture);
    if (s_Program)      glDeleteProgram(s_Program);
    if (s_EBO)          glDeleteBuffers(1, &s_EBO);
    if (s_VBO)          glDeleteBuffers(1, &s_VBO);
    if (s_VAO)          glDeleteVertexArrays(1, &s_VAO);

    s_WhiteTexture = 0;
    s_Program = s_EBO = s_VBO = s_VAO = 0;
}

void Renderer2D::StartBatch()
{
    s_IndexCount = 0;
    s_BufferPtr = s_BufferBase;
    s_TextureSlotCount = 1; // 0: white
}

void Renderer2D::Flush()
{
    if (s_IndexCount == 0) return;

    const GLsizeiptr size = (GLsizeiptr)((uint8_t*)s_BufferPtr - (uint8_t*)s_BufferBase);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, s_BufferBase);

    for (int i = 0; i < s_TextureSlotCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, s_TextureSlots[i]);
    }

    glUseProgram(s_Program);
    glBindVertexArray(s_VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)s_IndexCount, GL_UNSIGNED_INT, 0);

    StartBatch();
}

void Renderer2D::BeginScene(const glm::mat4& proj, const glm::mat4& view)
{
    s_Proj = proj;
    s_View = view;
    glUseProgram(s_Program);
    const glm::mat4 mvp = s_Proj * s_View * glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(s_Program, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    StartBatch();
}

void Renderer2D::DrawSprite(const SpriteDesc& s)
{
    // Texture slot bul/ekle
    float texIndex = 0.0f;
    if (s.texture) {
        int found = -1;
        for (int i = 1; i < s_TextureSlotCount; ++i) {
            if (s_TextureSlots[i] == s.texture) { found = i; break; }
        }
        if (found == -1) {
            if (s_TextureSlotCount >= s_MaxTextureSlots) Flush();
            found = s_TextureSlotCount;
            s_TextureSlots[s_TextureSlotCount++] = s.texture;
        }
        texIndex = (float)found;
    }

    if (s_IndexCount + 6 > (uint32_t)s_MaxIndices) Flush();

    const glm::vec2 p = s.pos;
    const glm::vec2 sz = s.size;

    // (rotation/origin uygulanmýyor -> düz axis-aligned quad)
    const glm::vec2 p0 = p;
    const glm::vec2 p1 = { p.x + sz.x, p.y };
    const glm::vec2 p2 = { p.x + sz.x, p.y + sz.y };
    const glm::vec2 p3 = { p.x,        p.y + sz.y };

    const glm::vec2 uv0 = { 0, 0 };
    const glm::vec2 uv1 = { 1, 0 };
    const glm::vec2 uv2 = { 1, 1 };
    const glm::vec2 uv3 = { 0, 1 };

    const glm::vec4 col = s.tint;

    s_BufferPtr[0] = { p0, uv0, col, texIndex };
    s_BufferPtr[1] = { p1, uv1, col, texIndex };
    s_BufferPtr[2] = { p2, uv2, col, texIndex };
    s_BufferPtr[3] = { p3, uv3, col, texIndex };
    s_BufferPtr += 4;

    s_IndexCount += 6;
}

void Renderer2D::DrawSpriteUV(const SpriteUVDesc& s)
{
    float texIndex = 0.0f;
    if (s.texture) {
        int found = -1;
        for (int i = 1; i < s_TextureSlotCount; ++i) {
            if (s_TextureSlots[i] == s.texture) { found = i; break; }
        }
        if (found == -1) {
            if (s_TextureSlotCount >= s_MaxTextureSlots) Flush();
            found = s_TextureSlotCount;
            s_TextureSlots[s_TextureSlotCount++] = s.texture;
        }
        texIndex = (float)found;
    }

    if (s_IndexCount + 6 > (uint32_t)s_MaxIndices) Flush();

    const glm::vec2 p = s.pos;
    const glm::vec2 sz = s.size;

    const glm::vec2 p0 = p;
    const glm::vec2 p1 = { p.x + sz.x, p.y };
    const glm::vec2 p2 = { p.x + sz.x, p.y + sz.y };
    const glm::vec2 p3 = { p.x,        p.y + sz.y };

    const glm::vec2 uv0 = s.uvMin;
    const glm::vec2 uv1 = { s.uvMax.x, s.uvMin.y };
    const glm::vec2 uv2 = s.uvMax;
    const glm::vec2 uv3 = { s.uvMin.x, s.uvMax.y };

    const glm::vec4 col = s.tint;

    s_BufferPtr[0] = { p0, uv0, col, texIndex };
    s_BufferPtr[1] = { p1, uv1, col, texIndex };
    s_BufferPtr[2] = { p2, uv2, col, texIndex };
    s_BufferPtr[3] = { p3, uv3, col, texIndex };
    s_BufferPtr += 4;

    s_IndexCount += 6;
}

void Renderer2D::EndScene()
{
    Flush();
}

GLuint Renderer2D::LoadTexture(const char* relativePng)
{
    return Assets::GetTexture(relativePng);
}

// UI: ekran piksel koordinatý ile quad çizer; (0,0) sol-üst
void Renderer2D::DrawScreenQuad(float x, float y, float w, float h,
    GLuint tex, const float tint[4])
{
    SpriteDesc s{};
    s.texture = tex;

    const float cx = x + w * 0.5f;
    const float cy = (g_ScreenH - y) - h * 0.5f; // sol-üst -> dünya

    s.pos = { cx, cy };
    s.size = { w, h };
    s.tint = { tint[0], tint[1], tint[2], tint[3] };

    DrawSprite(s);
}
