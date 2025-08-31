#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// stb_image: TEK bir .cpp dosyasýnda tanýmlanmalý
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

#ifdef _WIN32
#include <windows.h>
#endif

// ----------------- yardýmcýlar -----------------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

static std::string ReadFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cerr << "Dosya acilamadi: " << path << "\n"; return {}; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

static GLuint Compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(s, 2048, nullptr, log);
        std::cerr << "Shader derleme hatasi:\n" << log << "\n";
    }
    return s;
}

static GLuint MakeProgram(const char* vpath, const char* fpath) {
    std::string vsrc = ReadFile(vpath);
    std::string fsrc = ReadFile(fpath);
    GLuint vs = Compile(GL_VERTEX_SHADER, vsrc.c_str());
    GLuint fs = Compile(GL_FRAGMENT_SHADER, fsrc.c_str());
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetProgramInfoLog(p, 2048, nullptr, log);
        std::cerr << "Program link hatasi:\n" << log << "\n";
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

static GLuint LoadTexture2D(const char* path) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &n, 4);
    if (!data) {
        std::cerr << "Texture yuklenemedi: " << path << "\n";
        std::cerr << "Sebep: " << stbi_failure_reason() << "\n";
        return 0;
    }

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
// ------------------------------------------------

int main() {
    // GLFW + context
    if (!glfwInit()) { std::cerr << "GLFW init fail\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "MakeMyEngine - Sprite & Ortho", nullptr, nullptr);
    if (!win) { std::cerr << "Window create fail\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "GLAD load fail\n"; return -1; }
    std::cout << "GL: " << glGetString(GL_VERSION) << "\n";

    // --- ÇALIÞMA DÝZÝNÝNÝ EXE KLASÖRÜNE AL (assets/ için güvenli) ---
#ifdef _WIN32
    {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        std::filesystem::current_path(exeDir);
    }
#endif
    std::cout << "CWD: " << std::filesystem::current_path() << "\n";
    std::cout << "shader exists? " << std::filesystem::exists("assets/shaders/basic.vert") << "\n";
    std::cout << "texture exists? " << std::filesystem::exists("assets/texture.png") << "\n";

    // Quad (pos, uv)
    float verts[] = {
        -0.5f,-0.5f, 0.f,0.f,
         0.5f,-0.5f, 1.f,0.f,
         0.5f, 0.5f, 1.f,1.f,
        -0.5f, 0.5f, 0.f,1.f
    };
    unsigned int idx[] = { 0,1,2, 2,3,0 };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Shader program (assets, ileri eðik çizgi!)
    GLuint prog = MakeProgram("assets/shaders/basic.vert",
        "assets/shaders/sprite.frag");

    // Texture (assets/texture.png)
    GLuint tex = LoadTexture2D("assets/texture.png");

    // Ortho kamera (piksel koordinat)
    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
    glm::mat4 proj = glm::ortho(0.0f, (float)fbw, 0.0f, (float)fbh);
    glm::mat4 view = glm::mat4(1.0f);

    // Sprite transform (px)
    glm::vec2 pos = { 280.0f, 220.0f };
    glm::vec2 size = { 256.0f, 256.0f };

    while (!glfwWindowShouldClose(win)) {
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(pos + size * 0.5f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        glm::mat4 mvp = proj * view * model;

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, GL_FALSE, &mvp[0][0]);
        glUniform4f(glGetUniformLocation(prog, "uTint"), 1, 1, 1, 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(prog, "uTex"), 0);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteTextures(1, &tex);
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
