#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init fail\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "MakeMyEngine - Renderer2D", nullptr, nullptr);
    if (!win) { std::cerr << "Window create fail\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "GLAD load fail\n"; return -1; }
    std::cout << "GL: " << glGetString(GL_VERSION) << "\n";

#ifdef _WIN32
    { // CWD'yi exe klasörüne al
        char exePath[MAX_PATH]; GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
    }
#endif

    Renderer2D::Init();

    // texture'ler
    GLuint tex1 = Renderer2D::LoadTexture("texture.png");
    GLuint tex2 = std::filesystem::exists("assets/texture2.png") ? Renderer2D::LoadTexture("texture2.png") : tex1;

    // sahne objeleri
    SpriteDesc a{ tex1, {340,240}, {256,256}, {1,1,1,1} };
    SpriteDesc b{ tex2, {700,320}, {128,128}, {1,0.8f,0.8f,1} };
    SpriteDesc c{ tex1, {100,100}, { 96, 96}, {0.7f,1,0.7f,1} };

    while (!glfwWindowShouldClose(win)) {
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
        glm::mat4 proj = glm::ortho(0.0f, (float)fbw, 0.0f, (float)fbh);
        glm::mat4 view(1.0f);

        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Renderer2D::BeginScene(proj, view);
        Renderer2D::DrawSprite(a);
        Renderer2D::DrawSprite(b);
        Renderer2D::DrawSprite(c);
        Renderer2D::EndScene();

        // küçük hareket
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) a.pos.x += 200.f * (1.f / 60.f);
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) a.pos.x -= 200.f * (1.f / 60.f);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    Renderer2D::Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
