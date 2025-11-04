#pragma once
#include "Scene.h"
#include <glad/glad.h>

class GameScene : public Scene {
public:
    GameScene();
    ~GameScene();
    
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    
    void SetupGame(float worldWidth, float worldHeight);
    void SetWorldSize(float worldWidth, float worldHeight);
    void ResetBall();
    
    int GetPlayer1Score() const { return m_Player1Score; }
    int GetPlayer2Score() const { return m_Player2Score; }
    bool IsGameOver() const { return m_GameOver; }
    int GetWinningSide() const { return m_WinningSide; } // -1 = left (green), 1 = right (red), 0 = none
    void Restart();
    
private:
    void CreatePaddles(float worldWidth, float worldHeight);
    void CreateBall(float worldWidth, float worldHeight);
    void CreateWalls(float worldWidth, float worldHeight);
    void OnBallGoal(int side);
    
    std::shared_ptr<Entity> m_Paddle1;
    std::shared_ptr<Entity> m_Paddle2;
    std::shared_ptr<Entity> m_Ball;
    
    int m_Player1Score = 0;
    int m_Player2Score = 0;
    
    float m_WorldWidth = 1280.0f;
    float m_WorldHeight = 720.0f;

    bool m_WaitingServe = true;
    bool m_GameOver = false;
    int  m_WinningSide = 0;
    int  m_TargetScore = 5;
};

