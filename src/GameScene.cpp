#include "GameScene.h"
#include "Entity.h"
#include "SpriteComponent.h"
#include "AABBCollider.h"
#include "VelocityComponent.h"
#include "PaddleController.h"
#include "BallComponent.h"
#include "AssetManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Input.h"
#include <glm/glm.hpp>
#include <random>
#include <cmath>

GameScene::GameScene() : Scene("GameScene") {
}

GameScene::~GameScene() {
}

void GameScene::OnAttach() {
    SetupGame(m_WorldWidth, m_WorldHeight);
}

void GameScene::OnDetach() {
    Scene::OnDetach();
}

void GameScene::OnUpdate(float dt) {
    if (!m_GameOver) {
        Scene::OnUpdate(dt);
    }
    // Launch ball on SPACE (pressed or held) from center with 50/50 left/right
    if (m_Ball && m_WaitingServe && (Input::KeyPressed(GLFW_KEY_SPACE) || Input::KeyDown(GLFW_KEY_SPACE))) {
        auto vel = m_Ball->GetComponent<VelocityComponent>();
        auto ball = m_Ball->GetComponent<BallComponent>();
        if (vel && ball) {
            m_Ball->position = { m_WorldWidth * 0.5f, m_WorldHeight * 0.5f };
            static std::random_device rd; static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> angleDist(-0.5f, 0.5f);
            std::bernoulli_distribution side(0.5);
            float angle = angleDist(gen);
            float xDir = side(gen) ? 1.0f : -1.0f;
            glm::vec2 direction = { xDir, std::sin(angle) };
            ball->Launch(direction);
            m_WaitingServe = false;
        }
    }
    
    // Check ball-paddle collisions
    if (m_Ball && m_Paddle1 && m_Paddle2) {
        auto ballCollider = m_Ball->GetComponent<AABBCollider>();
        auto paddle1Collider = m_Paddle1->GetComponent<AABBCollider>();
        auto paddle2Collider = m_Paddle2->GetComponent<AABBCollider>();
        auto ballComponent = m_Ball->GetComponent<BallComponent>();
        
        if (ballCollider && ballComponent) {
            if (paddle1Collider && AABBCollider::CheckCollision(*ballCollider, *paddle1Collider)) {
                glm::vec2 normal = AABBCollider::GetCollisionNormal(*ballCollider, *paddle1Collider);
                ballComponent->Bounce(normal);
            }
            if (paddle2Collider && AABBCollider::CheckCollision(*ballCollider, *paddle2Collider)) {
                glm::vec2 normal = AABBCollider::GetCollisionNormal(*ballCollider, *paddle2Collider);
                ballComponent->Bounce(normal);
            }
        }
    }
}

void GameScene::OnRender() {
    Scene::OnRender();
}

void GameScene::SetupGame(float worldWidth, float worldHeight) {
    m_WorldWidth = worldWidth;
    m_WorldHeight = worldHeight;
    
    if (!m_Paddle1 || !m_Paddle2) {
        CreatePaddles(worldWidth, worldHeight);
    } else {
        // update paddle controller boundaries
        auto c1 = m_Paddle1->GetComponent<PaddleController>();
        if (c1) { c1->boundaryTop = 0.0f; c1->boundaryBottom = worldHeight; }
        auto c2 = m_Paddle2->GetComponent<PaddleController>();
        if (c2) { c2->boundaryTop = 0.0f; c2->boundaryBottom = worldHeight; }
    }
    if (!m_Ball) {
        CreateBall(worldWidth, worldHeight);
    } else {
        ResetBall();
    }
}

void GameScene::SetWorldSize(float worldWidth, float worldHeight) {
    m_WorldWidth = worldWidth;
    m_WorldHeight = worldHeight;
    auto c1 = m_Paddle1 ? m_Paddle1->GetComponent<PaddleController>() : nullptr;
    if (c1) { c1->boundaryTop = 0.0f; c1->boundaryBottom = worldHeight; }
    auto c2 = m_Paddle2 ? m_Paddle2->GetComponent<PaddleController>() : nullptr;
    if (c2) { c2->boundaryTop = 0.0f; c2->boundaryBottom = worldHeight; }
}

void GameScene::CreatePaddles(float worldWidth, float worldHeight) {
    GLuint whiteTex = Assets::GetWhiteTexture();
    float paddleWidth = 20.0f;
    float paddleHeight = 100.0f;
    float margin = 50.0f;
    
    // Left Paddle (Player 1)
    m_Paddle1 = CreateEntity("Paddle1");
    m_Paddle1->position = { margin + paddleWidth * 0.5f, worldHeight * 0.5f };
    m_Paddle1->zLayer = 1.0f;
    
    auto sprite1 = m_Paddle1->AddComponent<SpriteComponent>();
    sprite1->texture = whiteTex;
    sprite1->size = { paddleWidth, paddleHeight };
    sprite1->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    auto collider1 = m_Paddle1->AddComponent<AABBCollider>();
    collider1->size = { paddleWidth, paddleHeight };
    collider1->isStatic = false;
    
    auto velocity1 = m_Paddle1->AddComponent<VelocityComponent>();
    velocity1->maxSpeed = 500.0f;
    
    auto controller1 = m_Paddle1->AddComponent<PaddleController>();
    controller1->upKey = GLFW_KEY_W;
    controller1->downKey = GLFW_KEY_S;
    controller1->boundaryTop = 0.0f;
    controller1->boundaryBottom = worldHeight;
    
    // Right Paddle (Player 2)
    m_Paddle2 = CreateEntity("Paddle2");
    m_Paddle2->position = { worldWidth - margin - paddleWidth * 0.5f, worldHeight * 0.5f };
    m_Paddle2->zLayer = 1.0f;
    
    auto sprite2 = m_Paddle2->AddComponent<SpriteComponent>();
    sprite2->texture = whiteTex;
    sprite2->size = { paddleWidth, paddleHeight };
    sprite2->tint = { 1.0f, 1.0f, 1.0f, 1.0f };
    
    auto collider2 = m_Paddle2->AddComponent<AABBCollider>();
    collider2->size = { paddleWidth, paddleHeight };
    collider2->isStatic = true; // Paddles don't move from collisions
    
    auto velocity2 = m_Paddle2->AddComponent<VelocityComponent>();
    velocity2->maxSpeed = 500.0f;
    
    auto controller2 = m_Paddle2->AddComponent<PaddleController>();
    controller2->upKey = GLFW_KEY_UP;
    controller2->downKey = GLFW_KEY_DOWN;
    controller2->boundaryTop = 0.0f;
    controller2->boundaryBottom = worldHeight;
}

void GameScene::CreateBall(float worldWidth, float worldHeight) {
    if (m_Ball) return;
    GLuint whiteTex = Assets::GetWhiteTexture();
    float ballSize = 20.0f;
    
    m_Ball = CreateEntity("Ball");
    m_Ball->position = { worldWidth * 0.5f, worldHeight * 0.5f };
    m_Ball->zLayer = 0.5f;
    
    auto sprite = m_Ball->AddComponent<SpriteComponent>();
    sprite->texture = whiteTex;
    sprite->size = { ballSize, ballSize };
    sprite->tint = { 1.0f, 0.5f, 0.5f, 1.0f };
    
    auto collider = m_Ball->AddComponent<AABBCollider>();
    collider->size = { ballSize, ballSize };
    collider->isStatic = false;
    
    auto velocity = m_Ball->AddComponent<VelocityComponent>();
    velocity->maxSpeed = 600.0f;
    
    auto ball = m_Ball->AddComponent<BallComponent>();
    ball->speed = 300.0f;
    ball->onGoal = [this](int side) { OnBallGoal(side); };
    // Do not auto-launch; wait for SPACE
    ResetBall();
}

void GameScene::CreateWalls(float worldWidth, float worldHeight) {
    // Walls are handled by BallComponent's boundary checks
    // No need for wall entities if we use boundary checks
}

void GameScene::OnBallGoal(int side) {
    if (side < 0) {
        m_Player2Score++;
    } else {
        m_Player1Score++;
    }
    // Check win condition
    if (m_Player1Score >= m_TargetScore) {
        m_GameOver = true; m_WinningSide = -1; // left/green
    } else if (m_Player2Score >= m_TargetScore) {
        m_GameOver = true; m_WinningSide = 1; // right/red
    }
    ResetBall();
}

void GameScene::ResetBall() {
    if (!m_Ball) return;
    
    m_Ball->position = { m_WorldWidth * 0.5f, m_WorldHeight * 0.5f };
    
    auto ball = m_Ball->GetComponent<BallComponent>();
    auto velocity = m_Ball->GetComponent<VelocityComponent>();
    
    if (ball && velocity) {
        velocity->SetVelocity({ 0.0f, 0.0f });
        m_WaitingServe = true;
    }
}

void GameScene::Restart() {
    m_Player1Score = 0;
    m_Player2Score = 0;
    m_GameOver = false;
    m_WinningSide = 0;
    m_WaitingServe = true;
    ResetBall();
}

