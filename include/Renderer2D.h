#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

struct SpriteDesc {
    GLuint texture = 0;
    glm::vec2 pos = { 0,0 };      // px, sol-alt
    glm::vec2 size = { 100,100 };  // px
    glm::vec4 tint = { 1,1,1,1 };  // RGBA çarpaný
};

struct SpriteUVDesc {
    GLuint texture = 0;
    glm::vec2 pos = { 0,0 };
    glm::vec2 size = { 100,100 };
    glm::vec4 tint = { 1,1,1,1 };
    glm::vec2 uvMin = { 0,0 };   // alt-sol
    glm::vec2 uvMax = { 1,1 };   // üst-sað
};

class Renderer2D {
public:
    static void Init(int maxSprites = 10000);
    static void Shutdown();

    static void BeginScene(const glm::mat4& proj, const glm::mat4& view);
    static void DrawSprite(const SpriteDesc& s);     // tam doku
    static void DrawSpriteUV(const SpriteUVDesc& s); // atlas/alt-dikdörtgen
    static void EndScene();                          // flush

    static GLuint LoadTexture(const char* pathRelativeAssetsPNG); // "texture.png"

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
};
