#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "AssetManager.h"


static std::string ReadFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cerr << "Dosya acilamadi: " << path << "\n"; return {}; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
static GLuint Compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type); glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s, 2048, nullptr, log); std::cerr << "Shader derleme hatasi:\n" << log << "\n"; }
    return s;
}
static GLuint MakeProgramFromFiles(const char* v, const char* f) {
    std::string vs = ReadFile(v), fs = ReadFile(f);
    GLuint vsh = Compile(GL_VERTEX_SHADER, vs.c_str());
    GLuint fsh = Compile(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint p = glCreateProgram(); glAttachShader(p, vsh); glAttachShader(p, fsh); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(p, 2048, nullptr, log); std::cerr << "Program link hatasi:\n" << log << "\n"; }
    glDeleteShader(vsh); glDeleteShader(fsh); return p;
}

GLuint Renderer2D::s_VAO = 0, Renderer2D::s_VBO = 0, Renderer2D::s_EBO = 0, Renderer2D::s_Program = 0;
Renderer2D::QuadVertex* Renderer2D::s_BufferBase = nullptr;
Renderer2D::QuadVertex* Renderer2D::s_BufferPtr = nullptr;
uint32_t Renderer2D::s_IndexCount = 0;
int Renderer2D::s_MaxSprites = 0, Renderer2D::s_MaxVertices = 0, Renderer2D::s_MaxIndices = 0;
int Renderer2D::s_MaxTextureSlots = 0;
GLuint* Renderer2D::s_TextureSlots = nullptr;
int Renderer2D::s_TextureSlotCount = 0;
GLuint Renderer2D::s_WhiteTexture = 0;
glm::mat4 Renderer2D::s_Proj(1.f), Renderer2D::s_View(1.f);

void Renderer2D::Init(int maxSprites) {
    s_MaxSprites = maxSprites;
    s_MaxVertices = s_MaxSprites * 4;
    s_MaxIndices = s_MaxSprites * 6;

    GLint units = 0; glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &units);
    s_MaxTextureSlots = std::min(8, (int)units); // frag shader 8 slot

    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);

    glGenBuffers(1, &s_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, s_MaxVertices * sizeof(QuadVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, texIndex));

    std::vector<uint32_t> indices(s_MaxIndices);
    uint32_t offset = 0;
    for (int i = 0;i < s_MaxSprites;i++) {
        indices[i * 6 + 0] = offset + 0; indices[i * 6 + 1] = offset + 1; indices[i * 6 + 2] = offset + 2;
        indices[i * 6 + 3] = offset + 2; indices[i * 6 + 4] = offset + 3; indices[i * 6 + 5] = offset + 0;
        offset += 4;
    }
    glGenBuffers(1, &s_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    s_Program = Assets::GetShaderFromFiles("shaders/basic.vert", "shaders/sprite.frag");
    glUseProgram(s_Program);


    glGenTextures(1, &s_WhiteTexture);
    glBindTexture(GL_TEXTURE_2D, s_WhiteTexture);
    unsigned int white = 0xFFFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    s_TextureSlots = new GLuint[s_MaxTextureSlots];
    s_TextureSlots[0] = s_WhiteTexture;
    s_TextureSlotCount = 1;

    s_BufferBase = (QuadVertex*)malloc(s_MaxVertices * sizeof(QuadVertex));
    s_BufferPtr = s_BufferBase;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    StartBatch();
}

void Renderer2D::Shutdown() {
    free(s_BufferBase); s_BufferBase = nullptr;
    delete[] s_TextureSlots; s_TextureSlots = nullptr;
    glDeleteTextures(1, &s_WhiteTexture);
    glDeleteProgram(s_Program);
    glDeleteBuffers(1, &s_EBO);
    glDeleteBuffers(1, &s_VBO);
    glDeleteVertexArrays(1, &s_VAO);
}

void Renderer2D::StartBatch() {
    s_IndexCount = 0;
    s_BufferPtr = s_BufferBase;
    s_TextureSlotCount = 1; // 0: white
}

void Renderer2D::Flush() {
    if (s_IndexCount == 0) return;

    GLsizeiptr size = (uint8_t*)s_BufferPtr - (uint8_t*)s_BufferBase;
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, s_BufferBase);

    for (int i = 0;i < s_TextureSlotCount;i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, s_TextureSlots[i]);
    }

    glUseProgram(s_Program);
    glBindVertexArray(s_VAO);
    glDrawElements(GL_TRIANGLES, s_IndexCount, GL_UNSIGNED_INT, 0);

    StartBatch();
}

void Renderer2D::BeginScene(const glm::mat4& proj, const glm::mat4& view) {
    s_Proj = proj; s_View = view;
    glUseProgram(s_Program);
    glm::mat4 mvp = s_Proj * s_View * glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(s_Program, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    StartBatch();
}

void Renderer2D::DrawSprite(const SpriteDesc& s) {
    float texIndex = 0.0f;
    if (s.texture) {
        int found = -1;
        for (int i = 1;i < s_TextureSlotCount;i++)
            if (s_TextureSlots[i] == s.texture) { found = i; break; }
        if (found == -1) {
            if (s_TextureSlotCount >= s_MaxTextureSlots) Flush();
            found = s_TextureSlotCount;
            s_TextureSlots[s_TextureSlotCount++] = s.texture;
        }
        texIndex = (float)found;
    }
    if (s_IndexCount + 6 > (uint32_t)s_MaxIndices) Flush();

    glm::vec2 p = s.pos, sz = s.size;
    glm::vec2 p0 = p;
    glm::vec2 p1 = { p.x + sz.x, p.y };
    glm::vec2 p2 = { p.x + sz.x, p.y + sz.y };
    glm::vec2 p3 = { p.x,      p.y + sz.y };

    glm::vec2 uv0 = { 0,0 }, uv1 = { 1,0 }, uv2 = { 1,1 }, uv3 = { 0,1 };
    glm::vec4 col = s.tint;

    s_BufferPtr[0] = { p0, uv0, col, texIndex };
    s_BufferPtr[1] = { p1, uv1, col, texIndex };
    s_BufferPtr[2] = { p2, uv2, col, texIndex };
    s_BufferPtr[3] = { p3, uv3, col, texIndex };
    s_BufferPtr += 4;

    s_IndexCount += 6;
}

void Renderer2D::DrawSpriteUV(const SpriteUVDesc& s) {
    float texIndex = 0.0f;
    if (s.texture) {
        int found = -1;
        for (int i = 1;i < s_TextureSlotCount;i++)
            if (s_TextureSlots[i] == s.texture) { found = i; break; }
        if (found == -1) {
            if (s_TextureSlotCount >= s_MaxTextureSlots) Flush();
            found = s_TextureSlotCount;
            s_TextureSlots[s_TextureSlotCount++] = s.texture;
        }
        texIndex = (float)found;
    }
    if (s_IndexCount + 6 > (uint32_t)s_MaxIndices) Flush();

    glm::vec2 p = s.pos, sz = s.size;
    glm::vec2 p0 = p;
    glm::vec2 p1 = { p.x + sz.x, p.y };
    glm::vec2 p2 = { p.x + sz.x, p.y + sz.y };
    glm::vec2 p3 = { p.x,      p.y + sz.y };

    glm::vec2 uv0 = s.uvMin;
    glm::vec2 uv1 = { s.uvMax.x, s.uvMin.y };
    glm::vec2 uv2 = s.uvMax;
    glm::vec2 uv3 = { s.uvMin.x, s.uvMax.y };
    glm::vec4 col = s.tint;

    s_BufferPtr[0] = { p0, uv0, col, texIndex };
    s_BufferPtr[1] = { p1, uv1, col, texIndex };
    s_BufferPtr[2] = { p2, uv2, col, texIndex };
    s_BufferPtr[3] = { p3, uv3, col, texIndex };
    s_BufferPtr += 4;

    s_IndexCount += 6;
}

void Renderer2D::EndScene() { Flush(); }

GLuint Renderer2D::LoadTexture(const char* rel) {
    return Assets::GetTexture(rel); // rel: "texture.png" vb.
}

