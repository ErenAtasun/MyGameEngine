#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

// Ekran boyutlar� ba�ka bir .cpp�de TANIMLI olmal�
extern int g_ScreenW;
extern int g_ScreenH;

struct SpriteDesc {
    GLuint    texture = 0;
    glm::vec2 pos{ 0,0 };
    glm::vec2 size{ 1,1 };
    glm::vec4 tint{ 1,1,1,1 };
    float     z = 0.0f;
    float     rotation = 0.0f;
    glm::vec2 origin{ 0.5f, 0.5f };
};

struct SpriteUVDesc {
    GLuint    texture = 0;
    glm::vec2 pos{ 0,0 };
    glm::vec2 size{ 1,1 };
    glm::vec2 uvMin{ 0,0 };
    glm::vec2 uvMax{ 1,1 };
    glm::vec4 tint{ 1,1,1,1 };
    float     z = 0.0f;
    float     rotation = 0.0f;
    glm::vec2 origin{ 0.5f, 0.5f };
};

class Renderer2D {
public:
    static void Init(int maxSprites = 10000);
    static void Shutdown();

    static void BeginScene(const glm::mat4& proj, const glm::mat4& view);
    static void DrawSprite(const SpriteDesc& s);
    static void DrawSpriteUV(const SpriteUVDesc& s);
    static void EndScene();

    static GLuint LoadTexture(const char* relativePng);
    // UI i�in ekran koordinat�yla �izim
    static void DrawScreenQuad(float x, float y, float w, float h,
        GLuint tex, const float tint[4]);

    struct Stats { uint32_t drawCalls = 0; uint32_t quadCount = 0; uint32_t textureBinds = 0; };
    static void ResetStats();
    static Stats GetStats();

private:
    static void StartBatch();
    static void Flush();

    struct QuadVertex { glm::vec2 pos; glm::vec2 uv; glm::vec4 color; float texIndex; };

    static GLuint s_VAO, s_VBO, s_EBO, s_Program;
    static QuadVertex* s_BufferBase;
    static QuadVertex* s_BufferPtr;
    static uint32_t    s_IndexCount;

    static int s_MaxSprites, s_MaxVertices, s_MaxIndices;
    static int s_MaxTextureSlots;

    static GLuint* s_TextureSlots;
    static int     s_TextureSlotCount;
    static GLuint  s_WhiteTexture;

    static glm::mat4 s_Proj, s_View;

    static Stats s_Stats;
};
