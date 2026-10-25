#pragma once
#include <MME/MME.h>

class PongLayer : public mme::Layer {
public:
    PongLayer();

    void OnAttach() override;
    void OnFixedUpdate(float step) override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnUIRender() override;
    void OnEvent(mme::Event& e) override;

private:
    void BuildWorld();
    void LayoutWorld(float width, float height); // pencere boyutuna bağlı her şey
    void Serve();
    void ResetBall();
    void OnGoal(int side);
    void Restart();
    void Burst(glm::vec2 point, glm::vec2 direction, glm::vec4 color, int count, float speed);

    mme::Scene m_Scene{ "Pong" };
    mme::OrthoCamera m_Camera{ 1280.0f, 720.0f };
    glm::vec2 m_World{ 1280.0f, 720.0f };  // dünya = ekran pikseli, (0,0) sol-alt
    mme::ParticleSystem m_Particles{ 4000 }; // vuruş ve gol patlamaları

    mme::Entity* m_Paddle1 = nullptr;   // sol, yeşil oyuncu
    mme::Entity* m_Paddle2 = nullptr;   // sağ, kırmızı oyuncu
    mme::Entity* m_Ball = nullptr;
    mme::Entity* m_WallTop = nullptr;
    mme::Entity* m_WallBottom = nullptr;
    mme::Entity* m_GoalLeft = nullptr;
    mme::Entity* m_GoalRight = nullptr;

    int m_Score1 = 0;
    int m_Score2 = 0;
    int m_TargetScore = 5;
    bool m_WaitingServe = true;
    bool m_GameOver = false;
    int m_WinningSide = 0; // -1 sol (yeşil), 1 sağ (kırmızı)
    bool m_MusicOn = true;

    mme::UIButton m_BtnQuit;
    mme::UIButton m_BtnRestart;

    static constexpr float kPaddleWidth = 20.0f;
    static constexpr float kPaddleHeight = 100.0f;
    static constexpr float kPaddleMargin = 50.0f;
    static constexpr float kBallSize = 20.0f;
    static constexpr float kWallThickness = 200.0f;
};
