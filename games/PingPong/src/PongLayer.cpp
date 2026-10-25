#include "PongLayer.h"
#include "BallComponent.h"
#include "PaddleController.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
    const glm::vec4 kGreen{ 0.2f, 0.8f, 0.3f, 1.0f };
    const glm::vec4 kRed{ 0.8f, 0.2f, 0.3f, 1.0f };
    const glm::vec4 kWhite{ 1.0f, 1.0f, 1.0f, 1.0f };
    const glm::vec4 kBallColor{ 1.0f, 0.5f, 0.5f, 1.0f };
    const char* kMusic = "music/pong_loop.wav";

    // Oyunun kendi component'leri: F1 inspector'da düzenlenebilir, sahneyle kaydedilir
    void RegisterPongComponents() {
        using mme::ComponentRegistry;
        ComponentRegistry::Register<PaddleController>("PaddleController")
            .Field("upKey", &PaddleController::upKey)
            .Field("downKey", &PaddleController::downKey)
            .Field("moveSpeed", &PaddleController::moveSpeed)
            .Field("minY", &PaddleController::minY)
            .Field("maxY", &PaddleController::maxY);
        ComponentRegistry::Register<BallComponent>("BallComponent")
            .Field("baseSpeed", &BallComponent::baseSpeed)
            .Field("speedGain", &BallComponent::speedGain, 0.01f)
            .Field("maxSpeed", &BallComponent::maxSpeed)
            .Field("maxBounceAngleDeg", &BallComponent::maxBounceAngleDeg, 0.5f);
        ComponentRegistry::Register<Goal>("Goal")
            .Field("side", &Goal::side);
    }
}

PongLayer::PongLayer() : Layer("PongLayer") {
}

void PongLayer::OnAttach() {
    RegisterPongComponents();
    BuildWorld();

    m_BtnQuit.size = { 110.0f, 40.0f };
    m_BtnQuit.color = { 0.8f, 0.2f, 0.2f, 1.0f };
    m_BtnQuit.label = "ÇIKIŞ";
    m_BtnQuit.onClick = [] { mme::Application::Get().Close(); };

    m_BtnRestart.size = { 220.0f, 52.0f };
    m_BtnRestart.color = { 0.2f, 0.6f, 0.9f, 1.0f };
    m_BtnRestart.label = "YENİDEN BAŞLA";
    m_BtnRestart.onClick = [this] { Restart(); };

    const mme::Window& window = mme::Application::Get().GetWindow();
    LayoutWorld((float)window.GetWidth(), (float)window.GetHeight());

    mme::Audio::PlayMusic(kMusic, true, 0.35f);
}

void PongLayer::BuildWorld() {
    auto makePaddle = [&](const char* name, mme::KeyCode up, mme::KeyCode down) {
        mme::Entity& e = m_Scene.CreateEntity(name);
        e.position.y = m_World.y * 0.5f; // LayoutWorld bunu pencere yüksekliğine ölçekler
        e.zLayer = 1.0f;
        e.AddComponent<mme::SpriteComponent>().size = { kPaddleWidth, kPaddleHeight };
        e.AddComponent<mme::BoxCollider2D>().size = { kPaddleWidth, kPaddleHeight };
        e.AddComponent<mme::RigidBody2D>().type = mme::BodyType::Kinematic;
        auto& controller = e.AddComponent<PaddleController>();
        controller.upKey = up;
        controller.downKey = down;
        return &e;
    };
    m_Paddle1 = makePaddle("Paddle1", mme::Key::W, mme::Key::S);
    m_Paddle2 = makePaddle("Paddle2", mme::Key::Up, mme::Key::Down);

    m_Ball = &m_Scene.CreateEntity("Ball");
    m_Ball->zLayer = 0.5f;
    // İz, topun arkasında kalsın diye sprite'tan önce eklenir (önce çizilir)
    auto& trail = m_Ball->AddComponent<mme::ParticleEmitter>();
    trail.rate = 90.0f;
    trail.lifetime = 0.35f;
    trail.sizeBegin = 14.0f;
    trail.sizeEnd = 0.0f;
    trail.velocityVariation = { 12.0f, 12.0f };
    trail.colorBegin = { kBallColor.r, kBallColor.g, kBallColor.b, 0.55f };
    trail.colorEnd = { 1.0f, 0.8f, 0.4f, 0.0f };
    trail.emitting = false;
    auto& sprite = m_Ball->AddComponent<mme::SpriteComponent>();
    sprite.size = { kBallSize, kBallSize };
    sprite.tint = kBallColor;
    auto& ballCollider = m_Ball->AddComponent<mme::BoxCollider2D>();
    ballCollider.size = { kBallSize, kBallSize };
    ballCollider.restitution = 1.0f;  // duvarlardan enerji kaybetmeden seker
    ballCollider.friction = 0.0f;     // sürtünme sekme açısını bozmasın
    auto& body = m_Ball->AddComponent<mme::RigidBody2D>();
    body.type = mme::BodyType::Dynamic;
    body.fixedRotation = true;        // kare top dönmesin
    body.bullet = true;

    auto& ball = m_Ball->AddComponent<BallComponent>();
    ball.onGoal = [this](int side) { OnGoal(side); };
    ball.onPaddleHit = [this, &ball](glm::vec2 point, glm::vec2 normal, float speed) {
        const float t = speed / ball.maxSpeed;           // hızlandıkça daha tiz ve daha gösterişli
        mme::Audio::Play("sounds/paddle.wav", 0.8f, 0.9f + 0.5f * t);
        Burst(point, normal, normal.x > 0.0f ? kGreen : kRed, 18 + (int)(30 * t), 220.0f + 260.0f * t);
    };
    ball.onWallHit = [this](glm::vec2 point, glm::vec2 normal) {
        mme::Audio::Play("sounds/wall.wav", 0.6f);
        Burst(point, normal, { 1.0f, 1.0f, 1.0f, 0.8f }, 8, 140.0f);
    };

    // Üst/alt duvarlar: kalın statik kutular (hızlı top içinden geçemesin)
    m_WallTop = &m_Scene.CreateEntity("WallTop");
    m_WallTop->AddComponent<mme::BoxCollider2D>();
    m_WallBottom = &m_Scene.CreateEntity("WallBottom");
    m_WallBottom->AddComponent<mme::BoxCollider2D>();

    // Kaleler: ekranın hemen dışındaki trigger alanları
    auto makeGoal = [&](const char* name, int side) {
        mme::Entity& e = m_Scene.CreateEntity(name);
        e.AddComponent<mme::BoxCollider2D>().isTrigger = true;
        e.AddComponent<Goal>().side = side;
        return &e;
    };
    m_GoalLeft = makeGoal("GoalLeft", -1);
    m_GoalRight = makeGoal("GoalRight", 1);
}

void PongLayer::LayoutWorld(float width, float height) {
    if (width <= 0.0f || height <= 0.0f) return; // simge durumu

    const float oldHeight = m_World.y;
    m_World = { width, height };

    // Dünya = ekran pikseli: kamera dünyanın ortasına bakar
    m_Camera.SetViewportSize(width, height);
    m_Camera.SetPosition(m_World * 0.5f);

    const float t = kWallThickness;
    m_WallTop->position = { width * 0.5f, height + t * 0.5f };
    m_WallBottom->position = { width * 0.5f, -t * 0.5f };
    m_WallTop->GetComponent<mme::BoxCollider2D>()->size = { width + 2.0f * t, t };
    m_WallBottom->GetComponent<mme::BoxCollider2D>()->size = { width + 2.0f * t, t };

    m_GoalLeft->position = { -t * 0.5f, height * 0.5f };
    m_GoalRight->position = { width + t * 0.5f, height * 0.5f };
    m_GoalLeft->GetComponent<mme::BoxCollider2D>()->size = { t, height + 2.0f * t };
    m_GoalRight->GetComponent<mme::BoxCollider2D>()->size = { t, height + 2.0f * t };

    // Raketin hareket aralığındaki yeri korunur: kenara yaslı olan kenarda,
    // ortadaki ortada kalır.
    const float halfH = kPaddleHeight * 0.5f;
    auto place = [&](mme::Entity* paddle, float x) {
        const float oldRange = std::max(1.0f, oldHeight - 2.0f * halfH);
        const float frac = std::clamp((paddle->position.y - halfH) / oldRange, 0.0f, 1.0f);
        paddle->position.x = x;
        paddle->position.y = halfH + frac * std::max(0.0f, height - 2.0f * halfH);
        auto* c = paddle->GetComponent<PaddleController>();
        c->minY = 0.0f;
        c->maxY = height;
    };
    place(m_Paddle1, kPaddleMargin + kPaddleWidth * 0.5f);
    place(m_Paddle2, width - kPaddleMargin - kPaddleWidth * 0.5f);

    if (m_WaitingServe) {
        ResetBall();
    } else {
        // Pencere küçüldüyse top duvarların dışında kalmasın
        const float halfBall = kBallSize * 0.5f;
        m_Ball->position.y = std::clamp(m_Ball->position.y, halfBall, std::max(halfBall, height - halfBall));
    }

    // UI
    m_BtnQuit.pos = { width - m_BtnQuit.size.x - 20.0f, 20.0f };
    m_BtnRestart.pos = { (width - m_BtnRestart.size.x) * 0.5f, height * 0.5f - 40.0f };
}

void PongLayer::Burst(glm::vec2 point, glm::vec2 direction, glm::vec4 color, int count, float speed) {
    mme::ParticleProps p;
    p.position = point;
    p.velocity = direction * speed;
    p.velocityVariation = glm::vec2(speed * 0.6f);
    p.colorBegin = color;
    p.colorEnd = { color.r, color.g, color.b, 0.0f };
    p.sizeBegin = 7.0f;
    p.sizeVariation = 3.0f;
    p.sizeEnd = 0.0f;
    p.lifetime = 0.45f;
    p.lifetimeVariation = 0.15f;
    p.rotationSpeed = 360.0f;
    m_Particles.Emit(p, count);
}

void PongLayer::Serve() {
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> angle(-0.5f, 0.5f);
    std::bernoulli_distribution toRight(0.5);

    m_Ball->position = m_World * 0.5f;
    const float a = angle(rng);
    const float dirX = toRight(rng) ? 1.0f : -1.0f;
    m_Ball->GetComponent<BallComponent>()->Launch({ dirX * std::cos(a), std::sin(a) });
    m_Ball->GetComponent<mme::ParticleEmitter>()->emitting = true;
    m_WaitingServe = false;
    mme::Audio::Play("sounds/serve.wav", 0.7f);
}

void PongLayer::ResetBall() {
    m_Ball->position = m_World * 0.5f;
    m_Ball->GetComponent<BallComponent>()->Stop();
    m_Ball->GetComponent<mme::ParticleEmitter>()->emitting = false;
    m_WaitingServe = true;
}

void PongLayer::OnGoal(int side) {
    // Sol kaleye giren top sağ oyuncunun sayısıdır
    if (side < 0) ++m_Score2;
    else          ++m_Score1;
    const glm::vec4 scorerColor = side < 0 ? kRed : kGreen;

    // Kale çizgisinden sahaya doğru büyük patlama
    Burst(m_Ball->position, { -(float)side, 0.0f }, scorerColor, 90, 420.0f);

    if (m_Score1 >= m_TargetScore)      { m_GameOver = true; m_WinningSide = -1; }
    else if (m_Score2 >= m_TargetScore) { m_GameOver = true; m_WinningSide = 1; }

    mme::Audio::Play(m_GameOver ? "sounds/win.wav" : "sounds/goal.wav", 0.8f);
    MME_INFO("Gol! Skor ", m_Score1, " - ", m_Score2, m_GameOver ? " (oyun bitti)" : "");
    ResetBall();
}

void PongLayer::Restart() {
    m_Score1 = m_Score2 = 0;
    m_GameOver = false;
    m_WinningSide = 0;
    m_Particles.Clear();
    ResetBall();
}

void PongLayer::OnFixedUpdate(float step) {
    if (!m_GameOver)
        m_Scene.FixedUpdate(step);
}

void PongLayer::OnUpdate(float dt) {
    using namespace mme;

    if (Input::KeyPressed(Key::Escape))
        Application::Get().Close();

    if (Input::KeyPressed(Key::M)) {
        m_MusicOn = !m_MusicOn;
        if (m_MusicOn) Audio::PlayMusic(kMusic, true, 0.35f);
        else           Audio::StopMusic();
    }

    m_Particles.Update(dt); // oyun bitince de patlama sönümlensin

    if (m_GameOver) {
        if (Input::KeyPressed(Key::Enter) || Input::KeyPressed(Key::R))
            Restart();
        return;
    }

    if (m_WaitingServe && Input::KeyPressed(Key::Space))
        Serve();

    m_Scene.Update(dt);
}

void PongLayer::OnEvent(mme::Event& e) {
    if (e.type == mme::EventType::WindowResize)
        LayoutWorld((float)e.width, (float)e.height);
}

void PongLayer::OnRender() {
    mme::Renderer2D::BeginScene(m_Camera);

    // Orta çizgi
    const float dash = 18.0f, gap = 14.0f;
    for (float y = gap * 0.5f; y < m_World.y; y += dash + gap)
        mme::Renderer2D::DrawQuad({ m_World.x * 0.5f, y + dash * 0.5f }, { 4.0f, dash }, { 1.0f, 1.0f, 1.0f, 0.15f });

    m_Scene.Render();
    m_Particles.Render();
    mme::Renderer2D::EndScene();
}

void PongLayer::OnUIRender() {
    using mme::Renderer2D;
    const float W = m_World.x, H = m_World.y;

    // Oyun sonu karartması skorların altında kalsın diye önce çiziliyor
    if (m_GameOver)
        Renderer2D::DrawScreenRect({ 0.0f, 0.0f }, { W, H }, { 0.0f, 0.0f, 0.0f, 0.6f });

    // Skor: her sayı bir kare
    const float sq = 30.0f, gap = 5.0f, y = 80.0f;
    for (int i = 0; i < std::min(m_Score1, 10); ++i)
        Renderer2D::DrawScreenRect({ 100.0f + i * (sq + gap), y }, { sq, sq }, kGreen);
    for (int i = 0; i < std::min(m_Score2, 10); ++i)
        Renderer2D::DrawScreenRect({ W - 100.0f - sq - i * (sq + gap), y }, { sq, sq }, kRed);

    if (!m_GameOver && m_WaitingServe) {
        mme::UI::TextCentered("Servis için SPACE'e bas", { W * 0.5f, H - 80.0f }, 30.0f, kWhite);
        mme::UI::TextCentered("W/S ve Yukarı/Aşağı: raketler   ·   M: müzik   ·   F1: debug paneli   ·   F3: istatistik",
                              { W * 0.5f, H - 40.0f }, 18.0f, { 1.0f, 1.0f, 1.0f, 0.55f });
    }

    if (m_GameOver) {
        const glm::vec2 bannerSize{ 520.0f, 84.0f };
        const glm::vec2 bannerPos{ (W - bannerSize.x) * 0.5f, H * 0.5f - 140.0f };
        Renderer2D::DrawScreenRect(bannerPos, bannerSize, m_WinningSide < 0 ? kGreen : kRed);
        mme::UI::TextCentered(m_WinningSide < 0 ? "YEŞİL KAZANDI!" : "KIRMIZI KAZANDI!", bannerPos + bannerSize * 0.5f, 50.0f, kWhite);
        mme::UI::Button(m_BtnRestart);
    }

    mme::UI::Button(m_BtnQuit);
}
