#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

// ---- basit yardýmcýlar ----
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
    std::string vsrc = ReadFile(v), fsrc = ReadFile(f);
    GLuint vs = Compile(GL_VERTEX_SHADER, vsrc.c_str());
    GLuint fs = Compile(GL_FRAGMENT_SHADER, fsrc.c_str());
    GLuint p = glCreateProgram(); glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[2048]; glGetProgramInfoLog(p, 2048, nullptr, log); std::cerr << "Program link hatasi:\n" << log << "\n"; }
    glDeleteShader(vs); glDeleteShader(fs); return p;
}

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

GLuint Renderer2D::s_VAO = 0; GLuint Renderer2D::s_VBO = 0; GLuint Renderer2D::s_EBO = 0;
GLuint Renderer2D::s_Program = 0;
glm::mat4 Renderer2D::s_Proj(1.0f), Renderer2D::s_View(1.0f);

void Renderer2D::Init() {
    // quad (pos,uv)
    float verts[] = {
        -0.5f,-0.5f, 0.f,0.f,
         0.5f,-0.5f, 1.f,0.f,
         0.5f, 0.5f, 1.f,1.f,
        -0.5f, 0.5f, 0.f,1.f
    };
    unsigned int idx[] = { 0,1,2, 2,3,0 };

    glGenVertexArrays(1, &s_VAO);
    glGenBuffers(1, &s_VBO);
    glGenBuffers(1, &s_EBO);

    glBindVertexArray(s_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // shaderlar: exe CWD = build/Debug ? kopyalanmýþ assets klasöründen okuyacaðýz
    s_Program = MakeProgramFromFiles("assets/shaders/basic.vert",
        "assets/shaders/sprite.frag");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::Shutdown() {
    glDeleteProgram(s_Program);
    glDeleteVertexArrays(1, &s_VAO);
    glDeleteBuffers(1, &s_VBO);
    glDeleteBuffers(1, &s_EBO);
}

void Renderer2D::BeginScene(const glm::mat4& proj, const glm::mat4& view) {
    s_Proj = proj; s_View = view;
    glUseProgram(s_Program);
}

void Renderer2D::DrawSprite(const SpriteDesc& s) {
    glm::mat4 model =
        glm::translate(glm::mat4(1.0f), glm::vec3(s.pos + s.size * 0.5f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(s.size, 1.0f));
    glm::mat4 mvp = s_Proj * s_View * model;

    glUseProgram(s_Program);
    glUniformMatrix4fv(glGetUniformLocation(s_Program, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
    glUniform4fv(glGetUniformLocation(s_Program, "uTint"), 1, &s.tint[0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glUniform1i(glGetUniformLocation(s_Program, "uTex"), 0);

    glBindVertexArray(s_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Renderer2D::EndScene() {
    glBindVertexArray(0);
    glUseProgram(0);
}

GLuint Renderer2D::LoadTexture(const char* rel) {
    std::string path = std::string("assets/") + rel; // örn: "texture.png"
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 4);
    if (!data) { std::cerr << "Texture yuklenemedi: " << path << "\nSebep: " << stbi_failure_reason() << "\n"; return 0; }
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}
