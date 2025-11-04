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
#include "Input.h"
#include "UI.h"
#include "GameScene.h"

#ifdef _WIN32
#include <windows.h>
#endif
extern int g_ScreenW;
extern int g_ScreenH;

// ---------- Callbacks ----------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); g_ScreenW = w; g_ScreenH = h; }

static OrthoCameraController* gCamCtrl = nullptr;
static void scroll_cb(GLFWwindow*, double /*xoff*/, double yoff) {
    if (gCamCtrl) gCamCtrl->OnScroll(yoff);
}


int main() {
    // -------- GLFW / Context --------
    if (!glfwInit()) { std::cerr << "[ERR] GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "MakeMyEngine - Ping Pong", nullptr, nullptr);
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
    // �al��ma dizinini .exe klas�r�ne �ek (assets yol sorunlar�n� �nler)
    {
        char exePath[MAX_PATH]; GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::filesystem::current_path(std::filesystem::path(exePath).parent_path());
    }
#endif

    // -------- Systems Init --------
    Input::Init(win);
    Assets::Init("assets");
    Renderer2D::Init(10000);

    int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
    OrthoCameraController camCtrl((float)fbw, (float)fbh);
    gCamCtrl = &camCtrl;
    
    // -------- Game Scene --------
    auto gameScene = std::make_shared<GameScene>();
    gameScene->OnAttach();
    gameScene->SetWorldSize((float)fbw, (float)fbh);

    // -------- UI (butonlar) --------
    UIButton btnQuit{ (float)fbw - 120.f, 20.f, 100.f, 40.f };
    btnQuit.tint[0] = 0.8f; btnQuit.tint[1] = 0.2f; btnQuit.tint[2] = 0.2f;
    btnQuit.onClick = [win]() { glfwSetWindowShouldClose(win, 1); };
    UIButton btnRestart{ (float)fbw * 0.5f - 90.f, (float)fbh * 0.5f + 40.f, 180.f, 50.f };
    btnRestart.tint[0] = 0.2f; btnRestart.tint[1] = 0.6f; btnRestart.tint[2] = 0.9f;
    btnRestart.onClick = [&]() { gameScene->Restart(); };

    // -------- Main Loop --------
    double lastTime = glfwGetTime();
    int prevFbw = fbw, prevFbh = fbh;
    float statsTimer = 0.0f;
    bool wireframe = false;
    while (!glfwWindowShouldClose(win)) {
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        double now = glfwGetTime(); float dt = float(now - lastTime); lastTime = now; statsTimer += dt;

        glfwGetFramebufferSize(win, &fbw, &fbh);
        if (fbw != prevFbw || fbh != prevFbh) {
            prevFbw = fbw; prevFbh = fbh;
            camCtrl.OnResize((float)fbw, (float)fbh);
            gameScene->SetWorldSize((float)fbw, (float)fbh);
        }
        glViewport(0, 0, fbw, fbh);
        camCtrl.OnUpdate(dt, win);
        
        // Update game scene
        gameScene->OnUpdate(dt);

        glClearColor(0.10f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto& cam = camCtrl.GetCamera();
        Renderer2D::ResetStats();
        // Game Scene rendering
        Renderer2D::BeginScene(cam.GetProjection(), cam.GetView());
        gameScene->OnRender();
        Renderer2D::EndScene();

        // UI cizimi (tam ekran ortografik projeksiyon)
        glm::mat4 uiProj = glm::ortho(0.0f, (float)fbw, (float)fbh, 0.0f);
        glm::mat4 uiView = glm::mat4(1.0f);
        Renderer2D::BeginScene(uiProj, uiView);
        UI::Begin((float)fbw, (float)fbh);
        
        // Score display (simple quads for now)
        GLuint whiteTex = Assets::GetWhiteTexture();
        float scoreSize = 30.0f;
        float scoreX1 = 100.0f, scoreX2 = (float)fbw - 200.0f;
        float scoreY = 50.0f;
        
        // Player 1 score (green)
        int score1 = gameScene->GetPlayer1Score();
        for (int i = 0; i < score1 && i < 10; ++i) {
            float c1[4] = { 0.2f, 0.8f, 0.3f, 1.0f };
            Renderer2D::DrawScreenQuad(scoreX1 + i * (scoreSize + 5.0f), scoreY, scoreSize, scoreSize, whiteTex, c1);
        }
        
        // Player 2 score (red)
        int score2 = gameScene->GetPlayer2Score();
        for (int i = 0; i < score2 && i < 10; ++i) {
            float c2[4] = { 0.8f, 0.2f, 0.3f, 1.0f };
            Renderer2D::DrawScreenQuad(scoreX2 - i * (scoreSize + 5.0f), scoreY, scoreSize, scoreSize, whiteTex, c2);
        }
        
        // Game Over overlay (draw first, then UI elements on top)
        if (gameScene->IsGameOver()) {
            float overlay[4] = { 0.0f, 0.0f, 0.0f, 0.6f };
            Renderer2D::DrawScreenQuad(0, 0, (float)fbw, (float)fbh, whiteTex, overlay);
            // Winner banner color
            int side = gameScene->GetWinningSide();
            float bannerColor[4] = { 1,1,1,1 };
            if (side < 0) { bannerColor[0]=0.2f; bannerColor[1]=0.8f; bannerColor[2]=0.3f; } // green
            else if (side > 0) { bannerColor[0]=0.8f; bannerColor[1]=0.2f; bannerColor[2]=0.3f; } // red
            Renderer2D::DrawScreenQuad((float)fbw*0.5f - 250.f, (float)fbh*0.5f - 140.f, 500.f, 80.f, whiteTex, bannerColor);
            // Winner text
            const char* msg = side < 0 ? "GREEN WINS" : (side > 0 ? "RED WINS" : "");
            float white[4] = {1,1,1,1};
            UI::DrawTextSimple(msg, (float)fbw*0.5f - 210.f, (float)fbh*0.5f - 125.f, 56.f, white);
        }
        
        // Restart button (draw outside game over block, always process input)
        if (gameScene->IsGameOver()) {
            btnRestart.x = (float)fbw * 0.5f - 90.f;
            btnRestart.y = (float)fbh * 0.5f - 40.f;
            // Update callback to ensure it's set
            btnRestart.onClick = [&gameScene]() { 
                std::cout << "[Restart] Button clicked!\n";
                if (gameScene) gameScene->Restart(); 
            };
            UI::DrawButton(btnRestart);
            // Direct hit-test fallback (in case button press/release sequence misses)
            const auto& m = Input::Mouse();
            bool inside = (m.x >= btnRestart.x && m.x <= btnRestart.x + btnRestart.w &&
                           m.y >= btnRestart.y && m.y <= btnRestart.y + btnRestart.h);
            if (inside && (m.leftPressed || m.leftDown)) {
                std::cout << "[Restart] Direct click fallback\n";
                gameScene->Restart();
            }
            // Raw GLFW fallback
            if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && inside) {
                std::cout << "[Restart] Raw mouse press\n";
                gameScene->Restart();
            }
            if (glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) {
                std::cout << "[Restart] Raw key press\n";
                gameScene->Restart();
            }
            // Keyboard fallback to restart
            if (Input::KeyPressed(GLFW_KEY_ENTER) || Input::KeyPressed(GLFW_KEY_R)) {
                std::cout << "[Restart] Keyboard shortcut\n";
                gameScene->Restart();
            }
        }

        UI::DrawButton(btnQuit);
        UI::End();
        Renderer2D::EndScene();

        if (statsTimer >= 1.0f) {
            auto st = Renderer2D::GetStats();
            std::cout << "[Stats] drawCalls=" << st.drawCalls << ", quads=" << st.quadCount << ", textureBinds=" << st.textureBinds << "\n";
            statsTimer = 0.0f;
        }
        glfwSwapBuffers(win);
        glfwPollEvents();
        Input::NewFrame();
    }

    // -------- Shutdown --------
    Renderer2D::Shutdown();
    Assets::Shutdown();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
