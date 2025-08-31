#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

struct SpriteDesc {
    GLuint texture = 0;
    glm::vec2 pos = { 0,0 };      // px, sol-alt
    glm::vec2 size = { 100,100 };  // px
    glm::vec4 tint = { 1,1,1,1 };  // RGBA
};

class Renderer2D {
public:
    static void Init();                 // vao/vbo/ebo + shader yükle
    static void Shutdown();             // kaynaklarý býrak

    static void BeginScene(const glm::mat4& proj, const glm::mat4& view);
    static void DrawSprite(const SpriteDesc& s); // tek draw (þimdilik)
    static void EndScene();

    // yardýmcý: “assets” altýndan 2D texture yükle (stb_image kullanýr)
    static GLuint LoadTexture(const char* pathRelativeAssetsPNG); // "texture.png" gibi

private:
    static GLuint s_VAO, s_VBO, s_EBO;
    static GLuint s_Program;            // basic.vert + sprite.frag
    static glm::mat4 s_Proj, s_View;
};
