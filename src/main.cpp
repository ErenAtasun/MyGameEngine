// src/main.cpp
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
#include "Tilemap.h"

#ifdef _WIN32
#include <windows.h>
#endif

// ----------------- Callbacks -----------------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

static OrthoCameraController* gCamCtrl = nullptr;
static void scroll_cb(GLFWwindow*, double /*xoff*/, double yoff) {
    if (gCamCtrl) gCamCtrl->OnScroll(yoff);
}

// Atlas grid’inden (col,row) ? uvMin/uvMax
static inline void TileUV(int col, int row, int cols, int rows,
    glm::vec2& uvMin, glm::vec2& uvMax) {
    float u0 = (float)col / cols;
    float v0 = (float)row / rows;
    float u1 = (float)(col + 1) / cols;
    float v1 = (float)(row + 1) / rows;
    uvMin = { u0, v0 };
    uvMax = { u1, v1 };
}

int main() {
    // --------------- GLFW / Context ---------------
    if (!glfwInit()) { std::cerr << "[ERR] GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "MakeMyEngine - Camera + Batch + Tilemap", nullptr, nullptr);
    if (!win) { std::cerr << "[ERR] Window create failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetScrollCallback(win, scroll_cb);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[ERR] GLAD load failed\n"; return -1;
    }
    std::cout << "GL: " << glGetString(GL_VERSION) << "\n";

#ifdef _WIN32
    // Çalýþma dizinini .exe klasörüne çek (assets yol sorunlarýný önler)
    {
        char exePath[MAX_PATH]; GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
    }
#endif

    // --------------- Systems Init ---------------
    Assets::Init("assets");
    Renderer2D::Init(10000);                 // batch kapasitesi

    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
    OrthoCameraController camCtrl((float)fbw, (float)fbh);
    gCamCtrl = &camCtrl;

    // --------------- TEXTURES (tekil tanim) ---------------
    GLuint tex1 = Assets::GetTexture("texture.png");
    GLuint tex2 = std::filesystem::exists("assets/texture2.png")
        ? Assets::GetTexture("texture2.png") : tex1;
    GLuint texAtlas = std::filesystem::exists("assets/atlas.png")
        ? Assets::GetTexture("atlas.png") : tex1;

    // --------------- Demo Sprites ---------------
    SpriteDesc sA{ tex1, {340,240}, {256,256}, {1,1,1,1} };
    SpriteDesc sB{ tex2, {700,320}, {128,128}, {1.0f,0.85f,0.85f,1} };
    SpriteDesc sC{ tex1, {100,100}, { 96, 96}, {0.7f,1.0f,0.7f,1} };

    // Grid batch örneði
    std::vector<SpriteDesc> grid;
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 18; ++x)
            grid.push_back(SpriteDesc{ tex1, { 20.f + x * 60.f, 20.f + y * 40.f }, { 48.f, 32.f }, {1,1,1,1} });

    // --------------- Atlas Animasyonu ---------------
    const int ATLAS_COLS = 4, ATLAS_ROWS = 4; // atlas düzenine göre deðiþtir
    SpriteUVDesc animS; animS.texture = texAtlas;
    animS.pos = { 1000, 320 }; animS.size = { 128,128 }; animS.tint = { 1,1,1,1 };
    int   animFrame = 0;        // 0..(cols*rows-1)
    float animTime = 0.0f;
    float animFps = 8.0f;

    // --------------- TILEMAP Katmanlari ---------------
    // Ortak parametreler
    TilemapDesc base;
    base.atlasTex = texAtlas;
    base.atlasCols = 4;         // atlas.png 4x4 ise 4
    base.atlasRows = 4;
    base.tileW = 64;            // atlas karenin px boyutu
    base.tileH = 64;
    base.originPx = { 50,50 };
    base.tint = { 1,1,1,1 };

    Tilemap bg, world, fg;
    {
        TilemapDesc d = base;
        bg.LoadCSV("maps/bg.csv", d);    // yoksa boþ kalýr
    }
    {
        TilemapDesc d = base;
        world.LoadCSV("maps/level1.csv", d);
    }
    {
        TilemapDesc d = base;
        // istersen üst katmaný biraz transparan yap: d.tint = {1,1,1,0.9f};
        fg.LoadCSV("maps/fg.csv", d);    // yoksa boþ kalýr
    }

    // --------------- Main Loop ---------------
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        // dt
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        // pencere / kamera
        glfwGetFramebufferSize(win, &fbw, &fbh);
        camCtrl.OnResize((float)fbw, (float)fbh);
        camCtrl.OnUpdate(dt, win);                 // WASD

        // sprite hareketi (ok tuslari)
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) sA.pos.x += 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) sA.pos.x -= 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) sA.pos.y += 200.f * dt;
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) sA.pos.y -= 200.f * dt;

        // atlas animasyonu
        animTime += dt;
        while (animTime >= 1.0f / animFps) {
            animTime -= 1.0f / animFps;
            animFrame = (animFrame + 1) % (ATLAS_COLS * ATLAS_ROWS);
        }
        int fx = animFrame % ATLAS_COLS;
        int fy = animFrame / ATLAS_COLS;
        TileUV(fx, fy, ATLAS_COLS, ATLAS_ROWS, animS.uvMin, animS.uvMax);

        // (Opsiyonel) R: texture reload, Y: shader reload
        if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) {
            tex1 = Assets::ReloadTexture("texture.png");
            sA.texture = tex1;
            for (auto& g : grid) g.texture = tex1;
            std::cout << "[Reload] texture.png\n";
        }

        // --------------- Draw ---------------
        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto& cam = camCtrl.GetCamera();
        Renderer2D::BeginScene(cam.GetProjection(), cam.GetView());

        // KATMAN SIRASI = Z-ORDER
        bg.Draw();                 // 1) en altta
        world.Draw();              // 2) orta

        // sprite’lar harita üstünde
        Renderer2D::DrawSprite(sA);
        Renderer2D::DrawSprite(sB);
        Renderer2D::DrawSprite(sC);
        for (auto& spr : grid) Renderer2D::DrawSprite(spr);

        // atlas animasyonu
        if (texAtlas) Renderer2D::DrawSpriteUV(animS);

        fg.Draw();                 // 3) en üstte

        Renderer2D::EndScene();

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // --------------- Shutdown ---------------
    Renderer2D::Shutdown();
    Assets::Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
