#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Renderer2D.h"
#include "OrthoCamera.h"
#include "OrthoCameraController.h"
#include "AssetManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

// --- Callbacks ---
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

static OrthoCameraController* gCamCtrl = nullptr;
static void scroll_cb(GLFWwindow*, double /*xoff*/, double yoff) {
    if (gCamCtrl) gCamCtrl->OnScroll(yoff);
}

// Atlas grid’inden UV hesapla (col,row) ? uvMin/uvMax
static inline void TileUV(int col, int row, int cols, int rows,
    glm::vec2& uvMin, glm::vec2& uvMax)
{
    float u0 = (float)col / cols;
    float v0 = (float)row / rows;
    float u1 = (float)(col + 1) / cols;
    float v1 = (float)(row + 1) / rows;
    uvMin = { u0, v0 };
    uvMax = { u1, v1 };
}

int main() {
    // --- GLFW / context ---
    if (!glfwInit()) { std::cerr << "GLFW init fail\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "MakeMyEngine - Camera + Batch + UV", nullptr, nullptr);
    if (!win) { std::cerr << "Window create fail\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetScrollCallback(win, scroll_cb);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "GLAD load fail\n"; return -1; }
    std::cout << "GL: " << glGetString(GL_VERSION) << "\n";

#ifdef _WIN32
    // Çalýþma dizinini exe klasörüne çek (assets eriþimi)
    { char exePath[MAX_PATH]; GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::filesystem::current_path(std::filesystem::path(exePath).parent_path()); }
#endif

    // --- Sistem init ---
    Assets::Init("assets");          // asset kökü
    Renderer2D::Init(10000);         // batch kapasitesi

    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
    OrthoCameraController camCtrl((float)fbw, (float)fbh);
    gCamCtrl = &camCtrl;

    // --- Tekil TANIMLAR (BUNLAR TEK KEZ OLMALI) ---
    // Eðer bu üç satýr aþaðýda tekrar varsa, o tekrarlarý sil.
    GLuint tex1 = Assets::GetTexture("texture.png");
    GLuint tex2 = std::filesystem::exists("assets/texture2.png")
        ? Assets::GetTexture("texture2.png") : tex1;
    GLuint texAtlas = std::filesystem::exists("assets/atlas.png")
        ? Assets::GetTexture("atlas.png") : tex1;

    // --- Örnek sprite’lar ---
    SpriteDesc a{ tex1, {340,240}, {256,256}, {1,1,1,1} };
    SpriteDesc b{ tex2, {700,320}, {128,128}, {1,0.8f,0.8f,1} };
    SpriteDesc c{ tex1, {100,100}, { 96, 96}, {0.7f,1,0.7f,1} };

    // Grid: batch testi
    std::vector<SpriteDesc> sprites;
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 18; ++x)
            sprites.push_back(SpriteDesc{ tex1, { 20.f + x * 60.f, 20.f + y * 40.f }, { 48.f, 32.f }, {1,1,1,1} });

    // Atlas animasyonu (4x4 varsayýlan)
    const int ATLAS_COLS = 4, ATLAS_ROWS = 4;
    SpriteUVDesc animS; animS.texture = texAtlas; animS.pos = { 1000, 320 }; animS.size = { 128,128 }; animS.tint = { 1,1,1,1 };
    int   animFrame = 0;
    float animTime = 0.0f;
    float animFps = 8.0f;

    // --- Döngü ---
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        // dt
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        // ekran boyutu & kamera
        glfwGetFramebufferSize(win, &fbw, &fbh);
        camCtrl.OnResize((float)fbw, (float)fbh);
        camCtrl.OnUpdate(dt, win); // WASD

        // animasyon
        animTime += dt;
        while (animTime >= 1.0f / animFps) {
            animTime -= 1.0f / animFps;
            animFrame = (animFrame + 1) % (ATLAS_COLS * ATLAS_ROWS);
        }
        int fx = animFrame % ATLAS_COLS;
        int fy = animFrame / ATLAS_COLS;
        TileUV(fx, fy, ATLAS_COLS, ATLAS_ROWS, animS.uvMin, animS.uvMax);

        // ok tuþlarý ile tek bir sprite’ý hareket ettir
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) a.pos.x += 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) a.pos.x -= 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) a.pos.y += 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) a.pos.y -= 200.f * dt;

        // çizim
        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto& cam = camCtrl.GetCamera();
        Renderer2D::BeginScene(cam.GetProjection(), cam.GetView());
        Renderer2D::DrawSprite(a);
        Renderer2D::DrawSprite(b);
        Renderer2D::DrawSprite(c);
        for (auto& s : sprites) Renderer2D::DrawSprite(s);
        Renderer2D::DrawSpriteUV(animS);
        Renderer2D::EndScene();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // kapat
    Renderer2D::Shutdown();
    Assets::Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
