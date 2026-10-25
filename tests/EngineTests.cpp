// Motorun GPU gerektirmeyen kısımları için birim testleri.
// Çalıştırma: ctest --test-dir build -C Debug   (ya da doğrudan EngineTests.exe)
#include <MME/MME.h>
#include "InputInternal.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <typeindex>
#include <string>
#include <vector>

// ---------------------------------------------------------------- mini test koşucusu
namespace {
    struct TestCase { const char* name; std::function<void()> fn; };
    std::vector<TestCase>& Registry() { static std::vector<TestCase> r; return r; }
    int g_Failures = 0;

    struct Registrar { Registrar(const char* n, std::function<void()> f) { Registry().push_back({ n, std::move(f) }); } };

    void Fail(const char* file, int line, const std::string& msg) {
        ++g_Failures;
        std::printf("    FAIL %s:%d  %s\n", file, line, msg.c_str());
    }
}

#define TEST(name) static void name(); static Registrar reg_##name(#name, name); static void name()
#define CHECK(cond) do { if (!(cond)) Fail(__FILE__, __LINE__, #cond); } while (0)
#define CHECK_NEAR(a, b, eps) do { const double va_ = (a), vb_ = (b); if (std::abs(va_ - vb_) > (eps)) \
    Fail(__FILE__, __LINE__, std::string(#a " ~= " #b "  (") + std::to_string(va_) + " vs " + std::to_string(vb_) + ")"); } while (0)

using namespace mme;

// ---------------------------------------------------------------- yardımcılar
namespace {
    constexpr float kStep = 1.0f / 60.0f;

    struct ContactCounter : Component {
        int collisions = 0, triggers = 0;
        glm::vec2 lastNormal{ 0.0f };
        void OnCollisionEnter(const Collision& c) override { ++collisions; lastNormal = c.normal; }
        void OnTriggerEnter(Entity&) override { ++triggers; }
    };

    Entity& MakeBox(Scene& s, const char* name, glm::vec2 pos, glm::vec2 size, BodyType type, bool withBody = true) {
        Entity& e = s.CreateEntity(name);
        e.position = pos;
        e.AddComponent<BoxCollider2D>().size = size;
        if (withBody) e.AddComponent<RigidBody2D>().type = type;
        return e;
    }

    Event KeyEvent(EventType t, KeyCode k, bool repeat = false) {
        Event e{ t };
        e.key = k;
        e.repeat = repeat;
        return e;
    }
}

// ---------------------------------------------------------------- Input
// Projedeki en kritik hata buradaydı: "bu frame basıldı" bilgisi okunmadan siliniyordu.
TEST(Input_PressedLastsUntilNextFrame) {
    Input::Internal::BeginFrame();
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyPressed, Key::Space));
    CHECK(Input::KeyPressed(Key::Space));
    CHECK(Input::KeyDown(Key::Space));

    Input::Internal::BeginFrame();           // sonraki frame: hâlâ basılı ama "yeni" değil
    CHECK(!Input::KeyPressed(Key::Space));
    CHECK(Input::KeyDown(Key::Space));

    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyReleased, Key::Space));
    CHECK(Input::KeyReleased(Key::Space));
    CHECK(!Input::KeyDown(Key::Space));
    Input::Internal::BeginFrame();
}

TEST(Input_RepeatDoesNotRetrigger) {
    Input::Internal::BeginFrame();
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyPressed, Key::A));
    Input::Internal::BeginFrame();
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyPressed, Key::A, true)); // basılı tutma tekrarı
    CHECK(!Input::KeyPressed(Key::A));
    CHECK(Input::KeyDown(Key::A));
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyReleased, Key::A));
    Input::Internal::BeginFrame();
}

TEST(Input_TapWithinOneFrame) {
    // Bas ve bırak aynı poll'da gelirse ikisi de görülmeli (hızlı tıklama kaybolmasın)
    Input::Internal::BeginFrame();
    Event down{ EventType::MouseButtonPressed };  down.button = Mouse::Left;
    Event up{ EventType::MouseButtonReleased };   up.button = Mouse::Left;
    Input::Internal::ProcessEvent(down);
    Input::Internal::ProcessEvent(up);
    CHECK(Input::MousePressed(Mouse::Left));
    CHECK(Input::MouseReleased(Mouse::Left));
    CHECK(!Input::MouseDown(Mouse::Left));
    Input::Internal::BeginFrame();
}

TEST(Input_OutOfRangeCodesAreIgnored) {
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyPressed, -1));
    Input::Internal::ProcessEvent(KeyEvent(EventType::KeyPressed, 99999));
    CHECK(!Input::KeyDown(-1));
    CHECK(!Input::KeyDown(99999));
}

// ---------------------------------------------------------------- Entity / Scene
TEST(Entity_ComponentLookupByType) {
    Scene scene;
    Entity& e = scene.CreateEntity("E");
    auto& sprite = e.AddComponent<SpriteComponent>();
    CHECK(e.GetComponent<SpriteComponent>() == &sprite);
    CHECK(e.GetComponent<RigidBody2D>() == nullptr);
    CHECK(&sprite.GetEntity() == &e);
}

TEST(Scene_DestroyIsDeferredUntilEndOfUpdate) {
    struct SelfDestruct : Component {
        void OnUpdate(float) override { GetEntity().Destroy(); }
    };
    Scene scene;
    scene.CreateEntity("Keep");
    scene.CreateEntity("Doomed").AddComponent<SelfDestruct>();
    CHECK(scene.GetEntityCount() == 2);
    scene.Update(kStep);
    CHECK(scene.GetEntityCount() == 1);
    CHECK(scene.FindEntity("Doomed") == nullptr);
    CHECK(scene.FindEntity("Keep") != nullptr);
}

TEST(Scene_EntityIdsAreUnique) {
    Scene scene;
    const uint32_t a = scene.CreateEntity().GetID();
    const uint32_t b = scene.CreateEntity().GetID();
    CHECK(a != b);
    CHECK(scene.FindEntity(b) != nullptr);
}

// ---------------------------------------------------------------- Fizik
TEST(BoxCollider_OverlapAndContains) {
    Scene scene;
    Entity& a = MakeBox(scene, "A", { 0, 0 }, { 10, 10 }, BodyType::Static, false);
    Entity& b = MakeBox(scene, "B", { 9, 0 }, { 10, 10 }, BodyType::Static, false);
    Entity& c = MakeBox(scene, "C", { 10, 0 }, { 10, 10 }, BodyType::Static, false); // kenar teması: çakışma değil
    auto& ca = *a.GetComponent<BoxCollider2D>();
    CHECK(ca.Overlaps(*b.GetComponent<BoxCollider2D>()));
    CHECK(!ca.Overlaps(*c.GetComponent<BoxCollider2D>()));
    CHECK(ca.Contains({ 4.9f, -4.9f }));
    CHECK(!ca.Contains({ 5.1f, 0.0f }));

    a.scale = { 2.0f, 1.0f };                       // collider ölçekle büyür
    CHECK_NEAR(ca.GetHalfExtents().x, 10.0, 1e-5);
}

TEST(Physics_IntegratesVelocityAndGravity) {
    Scene scene;
    Entity& e = MakeBox(scene, "Ball", { 0, 0 }, { 1, 1 }, BodyType::Dynamic);
    e.GetComponent<RigidBody2D>()->velocity = { 60.0f, 0.0f };
    scene.GetPhysics().gravity = { 0.0f, -600.0f };
    scene.FixedUpdate(kStep);
    CHECK_NEAR(e.position.x, 1.0, 1e-4);            // 60 px/s * 1/60 s
    CHECK_NEAR(e.GetComponent<RigidBody2D>()->velocity.y, -10.0, 1e-4);
}

TEST(Physics_StaticBodiesNeverMove) {
    Scene scene;
    Entity& wall = MakeBox(scene, "Wall", { 0, 0 }, { 10, 10 }, BodyType::Static);
    wall.GetComponent<RigidBody2D>()->velocity = { 100.0f, 0.0f };
    scene.FixedUpdate(kStep);
    CHECK_NEAR(wall.position.x, 0.0, 1e-6);
}

TEST(Physics_DynamicBouncesOffStaticWall) {
    Scene scene;
    MakeBox(scene, "Wall", { 0, 0 }, { 10, 100 }, BodyType::Static, false);
    Entity& ball = MakeBox(scene, "Ball", { 9, 0 }, { 10, 10 }, BodyType::Dynamic); // 1 px içeride
    auto& rb = *ball.GetComponent<RigidBody2D>();
    rb.velocity = { -60.0f, 30.0f };
    rb.restitution = 1.0f;
    auto& counter = ball.AddComponent<ContactCounter>();

    scene.FixedUpdate(kStep);
    CHECK(ball.position.x >= 10.0f - 1e-4f);        // dışarı itildi
    CHECK_NEAR(rb.velocity.x, 60.0, 1e-3);          // x yansıdı
    CHECK_NEAR(rb.velocity.y, 30.0, 1e-3);          // y korundu
    CHECK(counter.collisions == 1);
    CHECK_NEAR(counter.lastNormal.x, 1.0, 1e-6);    // duvardan topa bakıyor
}

TEST(Physics_CollisionEnterFiresOncePerContact) {
    Scene scene;
    MakeBox(scene, "Floor", { 0, -5 }, { 100, 10 }, BodyType::Static, false);
    Entity& box = MakeBox(scene, "Box", { 0, 5 }, { 10, 10 }, BodyType::Dynamic);
    scene.GetPhysics().gravity = { 0.0f, -600.0f };
    auto& counter = box.AddComponent<ContactCounter>();

    for (int i = 0; i < 30; ++i) scene.FixedUpdate(kStep); // zeminde yarım saniye durur
    CHECK(counter.collisions == 1);
    CHECK_NEAR(box.position.y, 5.0, 0.05);           // zeminin içine batmadı
}

TEST(Physics_TriggerReportsButDoesNotPush) {
    Scene scene;
    Entity& goal = MakeBox(scene, "Goal", { 0, 0 }, { 10, 10 }, BodyType::Static, false);
    goal.GetComponent<BoxCollider2D>()->isTrigger = true;
    Entity& ball = MakeBox(scene, "Ball", { 2, 0 }, { 2, 2 }, BodyType::Dynamic);
    ball.GetComponent<RigidBody2D>()->velocity = { -60.0f, 0.0f };
    auto& counter = ball.AddComponent<ContactCounter>();

    scene.FixedUpdate(kStep);
    scene.FixedUpdate(kStep);
    CHECK(counter.triggers == 1);
    CHECK(counter.collisions == 0);
    CHECK_NEAR(ball.position.x, 0.0, 1e-4);          // 2 adımda 2 px, itilmeden
}

TEST(Physics_KinematicIsNotPushedByDynamic) {
    Scene scene;
    Entity& paddle = MakeBox(scene, "Paddle", { 0, 0 }, { 10, 50 }, BodyType::Kinematic);
    Entity& ball = MakeBox(scene, "Ball", { 9, 0 }, { 10, 10 }, BodyType::Dynamic);
    ball.GetComponent<RigidBody2D>()->velocity = { -120.0f, 0.0f };
    scene.FixedUpdate(kStep);
    CHECK_NEAR(paddle.position.x, 0.0, 1e-6);
    CHECK(ball.position.x >= 10.0f - 1e-4f);
}

TEST(RigidBody_MaxSpeedClamp) {
    Scene scene;
    auto& rb = scene.CreateEntity().AddComponent<RigidBody2D>();
    rb.maxSpeed = 100.0f;
    rb.SetVelocity({ 300.0f, 400.0f });
    CHECK_NEAR(glm::length(rb.velocity), 100.0, 1e-3);
    CHECK_NEAR(rb.velocity.x / rb.velocity.y, 0.75, 1e-5); // yön korunur
}

// ---------------------------------------------------------------- Kamera
TEST(Camera_ScreenToWorld) {
    OrthoCamera cam(800.0f, 600.0f);
    cam.SetPosition({ 1000.0f, 500.0f });
    glm::vec2 c = cam.ScreenToWorld({ 400.0f, 300.0f });     // ekran ortası = kamera konumu
    CHECK_NEAR(c.x, 1000.0, 1e-2);
    CHECK_NEAR(c.y, 500.0, 1e-2);
    glm::vec2 tl = cam.ScreenToWorld({ 0.0f, 0.0f });       // sol-üst: -x, +y
    CHECK_NEAR(tl.x, 600.0, 1e-2);
    CHECK_NEAR(tl.y, 800.0, 1e-2);

    cam.SetZoom(2.0f);                                      // 2x uzaklaş: görünen alan 2 katı
    tl = cam.ScreenToWorld({ 0.0f, 0.0f });
    CHECK_NEAR(tl.x, 200.0, 1e-2);
    CHECK_NEAR(tl.y, 1100.0, 1e-2);
    CHECK_NEAR(cam.GetVisibleSize().x, 1600.0, 1e-3);
}

// ---------------------------------------------------------------- Tilemap
TEST(Tilemap_FirstCsvRowIsTop) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "mme_tests";
    fs::create_directories(dir);
    std::ofstream(dir / "map.csv") << "0,1,2\n3,-1,4\n5,6\n"; // son satır eksik: -1 ile tamamlanmalı

    Assets::Init(dir.string());
    Tilemap map;
    TilemapDesc desc;
    desc.tileW = desc.tileH = 10;
    desc.origin = { 100.0f, 0.0f };
    CHECK(map.LoadCSV("map.csv", desc));
    CHECK(map.GetColumns() == 3 && map.GetRows() == 3);
    CHECK(map.GetTile(2, 2) == -1);

    int col = -1, row = -1;
    CHECK(map.WorldToTile({ 105.0f, 25.0f }, col, row));   // en üst satır dünyada en yukarıda
    CHECK(col == 0 && row == 0 && map.GetTile(col, row) == 0);
    CHECK(map.WorldToTile({ 125.0f, 5.0f }, col, row));
    CHECK(col == 2 && row == 2);
    CHECK(!map.WorldToTile({ 99.0f, 5.0f }, col, row));    // harita dışı

    const glm::vec2 corner = map.TileToWorld(1, 0);
    CHECK_NEAR(corner.x, 110.0, 1e-5);
    CHECK_NEAR(corner.y, 20.0, 1e-5);
    fs::remove_all(dir);
}

// ---------------------------------------------------------------- UI
TEST(UI_MeasureText) {
    // 7 px font: her karakter 5 px + 1 px boşluk, son boşluk sayılmaz
    glm::vec2 s = UI::MeasureText("AB", 7.0f);
    CHECK_NEAR(s.x, 11.0, 1e-5);
    CHECK_NEAR(s.y, 7.0, 1e-5);
    s = UI::MeasureText("ABC\nA", 7.0f);
    CHECK_NEAR(s.x, 17.0, 1e-5);                    // en uzun satır
    CHECK_NEAR(s.y, 15.0, 1e-5);                    // 7 + 8 satır aralığı
    CHECK(UI::MeasureText("", 7.0f).x == 0.0f);
}

// ---------------------------------------------------------------- Fizik: broadphase ve alt adımlama
TEST(Physics_BroadphaseMatchesBruteForce) {
    // Kinematik kutular itilmez: her çakışan çift tam bir "enter" olayı üretir.
    // Uzamsal ızgaranın bulduğu temaslar kaba kuvvet O(n²) testiyle aynı olmalı.
    for (float cell : { 8.0f, 64.0f, 100000.0f }) {          // küçük, normal, tek dev hücre
        Scene scene;
        scene.GetPhysics().cellSize = cell;
        std::mt19937 rng(1234);
        std::uniform_real_distribution<float> pos(0.0f, 600.0f), size(4.0f, 60.0f);
        std::vector<Entity*> boxes;
        for (int i = 0; i < 250; ++i) {
            Entity& e = MakeBox(scene, "Box", { pos(rng), pos(rng) }, { size(rng), size(rng) }, BodyType::Kinematic);
            e.AddComponent<ContactCounter>();
            boxes.push_back(&e);
        }
        MakeBox(scene, "Huge", { 300, 300 }, { 5000, 5000 }, BodyType::Static, false); // ızgaraya sığmayan

        int boxPairs = 0;
        for (size_t i = 0; i < boxes.size(); ++i)
            for (size_t j = i + 1; j < boxes.size(); ++j)
                if (boxes[i]->GetComponent<BoxCollider2D>()->Overlaps(*boxes[j]->GetComponent<BoxCollider2D>()))
                    ++boxPairs;

        scene.FixedUpdate(kStep);
        int reported = 0;
        for (Entity* e : boxes) reported += e->GetComponent<ContactCounter>()->collisions;
        // Kutular arası çiftte iki taraf da sayar; dev kutuyla çakışma her kutu için bir kez
        CHECK(reported == boxPairs * 2 + (int)boxes.size());
        CHECK(scene.GetPhysics().GetStats().contacts == (uint32_t)(boxPairs + (int)boxes.size()));
    }
}

TEST(Physics_SubstepsPreventTunneling) {
    // 3000 px/s: bir adımda 50 px. 4 px'lik duvar ve 10 px'lik top için tespit
    // penceresi 14 px; alt adım olmadan top duvarın içinden geçer.
    auto run = [](int maxSubsteps) {
        Scene scene;
        scene.GetPhysics().maxSubsteps = maxSubsteps;
        MakeBox(scene, "Wall", { 100, 0 }, { 4, 200 }, BodyType::Static, false);
        Entity& ball = MakeBox(scene, "Ball", { 0, 0 }, { 10, 10 }, BodyType::Dynamic);
        auto& rb = *ball.GetComponent<RigidBody2D>();
        rb.velocity = { 3000.0f, 0.0f };
        rb.restitution = 1.0f;
        for (int i = 0; i < 4; ++i) scene.FixedUpdate(kStep);
        return std::make_pair(ball.position.x, rb.velocity.x);
    };
    const auto noSub = run(1);
    CHECK(noSub.first > 100.0f);            // alt adımsız: tünelledi (testin anlamlı olduğunun kanıtı)
    CHECK(noSub.second > 0.0f);
    const auto withSub = run(8);
    CHECK(withSub.first < 100.0f);          // alt adımlı: duvardan sekti
    CHECK_NEAR(withSub.second, -3000.0, 1e-2);
}

// ---------------------------------------------------------------- Component kaydı ve serileştirme
namespace {
    enum class Mood { Calm, Angry, Sleepy };

    struct TestComponent : Component {
        float f = 0.0f;
        int i = 0;
        bool b = false;
        glm::vec2 v{ 0.0f };
        glm::vec4 c{ 0.0f };
        std::string s;
        Mood mood = Mood::Calm;
    };

    void RegisterTestComponent() {
        ComponentRegistry::Register<TestComponent>("TestComponent")
            .Field("f", &TestComponent::f)
            .Field("i", &TestComponent::i)
            .Field("b", &TestComponent::b)
            .Field("v", &TestComponent::v)
            .Field("c", &TestComponent::c)
            .Field("s", &TestComponent::s)
            .Enum("mood", &TestComponent::mood, { "Calm", "Angry", "Sleepy" });
    }
}

TEST(Registry_FindAndFieldAccess) {
    ComponentRegistry::RegisterBuiltins();
    RegisterTestComponent();
    const size_t count = ComponentRegistry::GetAll().size();
    RegisterTestComponent();                              // tekrar kayıt çoğaltmamalı
    CHECK(ComponentRegistry::GetAll().size() == count);
    const ComponentInfo* info = ComponentRegistry::Find("TestComponent");
    CHECK(info != nullptr);
    if (!info) return;
    CHECK(info == ComponentRegistry::Find(std::type_index(typeid(TestComponent))));
    CHECK(info->fields.size() == 7);
    CHECK(ComponentRegistry::Find("RigidBody2D") != nullptr);

    Scene scene;
    Entity& e = scene.CreateEntity();
    Component& c = info->add(e);
    *static_cast<float*>(info->fields[0].access(c)) = 4.5f;
    CHECK_NEAR(e.GetComponent<TestComponent>()->f, 4.5, 1e-6);
    info->remove(e);
    CHECK(!e.HasComponent<TestComponent>());
}

TEST(Serializer_RoundTripPreservesFieldsAndOrder) {
    ComponentRegistry::RegisterBuiltins();
    RegisterTestComponent();

    Scene a("Kaynak");
    Entity& e = a.CreateEntity("Oyuncu Ş");                  // UTF-8 isim
    e.position = { 12.5f, -3.0f };
    e.rotation = 45.0f;
    e.scale = { 2.0f, 0.5f };
    e.zLayer = 3.0f;
    e.active = false;
    auto& emitter = e.AddComponent<ParticleEmitter>();       // önce eklenen önce gelmeli
    emitter.rate = 77.0f;
    emitter.colorBegin = { 0.1f, 0.2f, 0.3f, 0.4f };
    auto& rb = e.AddComponent<RigidBody2D>();
    rb.type = BodyType::Kinematic;
    rb.velocity = { 5.0f, 6.0f };
    rb.restitution = 0.25f;
    e.AddComponent<BoxCollider2D>().isTrigger = true;
    auto& t = e.AddComponent<TestComponent>();
    t.f = 1.5f; t.i = -7; t.b = true; t.v = { 8, 9 }; t.c = { 1, 0, 1, 0.5f }; t.s = "merhaba dünya"; t.mood = Mood::Sleepy;
    a.CreateEntity("Bos");

    const std::string jsonText = SceneSerializer::ToJson(a);
    Scene b("Hedef");
    b.CreateEntity("silinmeli");                              // yükleme önce temizler
    CHECK(SceneSerializer::FromJson(b, jsonText));
    CHECK(b.GetEntityCount() == 2);

    Entity* e2 = b.FindEntity("Oyuncu Ş");
    CHECK(e2 != nullptr);
    if (!e2) return;
    CHECK_NEAR(e2->position.x, 12.5, 1e-5);
    CHECK_NEAR(e2->rotation, 45.0, 1e-5);
    CHECK_NEAR(e2->scale.y, 0.5, 1e-5);
    CHECK_NEAR(e2->zLayer, 3.0, 1e-5);
    CHECK(!e2->active);

    std::vector<std::string> order;
    e2->ForEachComponent([&](std::type_index type, Component&) { order.push_back(ComponentRegistry::Find(type)->name); });
    CHECK(order.size() == 4 && order[0] == "ParticleEmitter" && order[3] == "TestComponent");

    CHECK_NEAR(e2->GetComponent<ParticleEmitter>()->rate, 77.0, 1e-5);
    CHECK_NEAR(e2->GetComponent<ParticleEmitter>()->colorBegin.w, 0.4, 1e-5);
    CHECK(e2->GetComponent<RigidBody2D>()->type == BodyType::Kinematic);
    CHECK_NEAR(e2->GetComponent<RigidBody2D>()->velocity.y, 6.0, 1e-5);
    CHECK(e2->GetComponent<BoxCollider2D>()->isTrigger);
    const auto* t2 = e2->GetComponent<TestComponent>();
    CHECK(t2 && t2->i == -7 && t2->b && t2->s == "merhaba dünya" && t2->mood == Mood::Sleepy);
    CHECK(t2 && t2->v.y == 9.0f && t2->c.w == 0.5f);
}

TEST(Serializer_RejectsBadInput) {
    Scene scene;
    scene.CreateEntity("Kalsin");
    CHECK(!SceneSerializer::FromJson(scene, "{ bozuk json"));
    CHECK(!SceneSerializer::FromJson(scene, R"({"format": 99, "entities": []})"));
    CHECK(scene.GetEntityCount() == 1);                       // başarısız yükleme sahneye dokunmaz
    // Bilinmeyen component atlanır, geri kalanı yüklenir
    CHECK(SceneSerializer::FromJson(scene, R"({"format":1,"entities":[{"name":"X","components":{"Yok":{},"BoxCollider2D":{"size":[3,4]}}}]})"));
    Entity* x = scene.FindEntity("X");
    CHECK(x && x->GetComponent<BoxCollider2D>() && x->GetComponent<BoxCollider2D>()->size.y == 4.0f);
}

// ---------------------------------------------------------------- Animasyon ve parçacıklar
TEST(SpriteAnimator_AdvancesAndFinishes) {
    Scene scene;
    Entity& e = scene.CreateEntity();
    auto& sprite = e.AddComponent<SpriteComponent>();
    auto& anim = e.AddComponent<SpriteAnimator>();
    anim.columns = 4;
    anim.rows = 2;
    int finished = 0;
    anim.onFinished = [&] { ++finished; };
    anim.AddClip("run", 4, 4, 10.0f, false);                 // alt satır
    anim.Play("run");
    CHECK(anim.GetSheetFrame() == 4);
    CHECK_NEAR(sprite.uvMin.y, 0.0, 1e-6);                   // alt satır = v'nin alt yarısı
    CHECK_NEAR(sprite.uvMax.y, 0.5, 1e-6);

    scene.Update(0.1f);
    CHECK(anim.GetSheetFrame() == 5);
    CHECK_NEAR(sprite.uvMin.x, 0.25, 1e-6);

    scene.Update(1.0f);                                       // sona kadar
    CHECK(anim.GetFrameInClip() == 3);
    CHECK(!anim.playing);
    CHECK(finished == 1);
    scene.Update(1.0f);
    CHECK(finished == 1);                                     // bir kez
}

TEST(ParticleSystem_LifetimeAndPoolOverflow) {
    ParticleSystem ps(5);
    ParticleProps p;
    p.lifetime = 0.5f;
    ps.Emit(p, 3);
    CHECK(ps.GetActiveCount() == 3);
    ps.Emit(p, 5);                                            // kapasite 5: en eskiler ezilir
    CHECK(ps.GetActiveCount() == 5);
    ps.Update(0.25f);
    CHECK(ps.GetActiveCount() == 5);
    ps.Update(0.3f);
    CHECK(ps.GetActiveCount() == 0);
}

// ---------------------------------------------------------------- Font ve UTF-8
TEST(Utf8_Decode) {
    const std::string s = "a\xC3\xA7\xC4\x9F\xE2\x82\xAC\xF0\x9F\x98\x80";  // a ç ğ € 😀
    size_t i = 0;
    CHECK(DecodeUtf8(s, i) == 'a');
    CHECK(DecodeUtf8(s, i) == 0x00E7);
    CHECK(DecodeUtf8(s, i) == 0x011F);
    CHECK(DecodeUtf8(s, i) == 0x20AC);
    CHECK(DecodeUtf8(s, i) == 0x1F600);
    CHECK(i == s.size());

    const std::string bad = "\x80" "A" "\xC3";               // tek devam baytı, sonra eksik dizi
    i = 0;
    CHECK(DecodeUtf8(bad, i) == 0xFFFD);
    CHECK(DecodeUtf8(bad, i) == 'A');
    CHECK(DecodeUtf8(bad, i) == 0xFFFD);
    CHECK(i == bad.size());
}

TEST(Font_BuiltinTransliteratesTurkish) {
    auto font = Font::CreateBuiltin();
    CHECK(!font->HasGlyph(0x015F));                           // ş yok...
    CHECK(&font->GetGlyph(0x015F) == &font->GetGlyph('s'));  // ...s olarak çizilir
    CHECK(&font->GetGlyph(0x0130) == &font->GetGlyph('I'));  // İ -> I
    CHECK(&font->GetGlyph(0x4E2D) == &font->GetGlyph('?'));  // 中 -> ?
    CHECK_NEAR(UI::MeasureText("şeker", 7.0f, font.get()).x, UI::MeasureText("seker", 7.0f, font.get()).x, 1e-5);
}

TEST(Font_TTFHasTurkishGlyphs) {
    Assets::Init(MME_TEST_ASSETS_DIR);
    auto font = Font::LoadTTF("fonts/FiraSans-Regular.ttf", 32.0f);
    CHECK(font != nullptr);
    if (!font) return;
    for (uint32_t cp : { 0x00E7u, 0x011Fu, 0x0131u, 0x00F6u, 0x015Fu, 0x00FCu, 0x00C7u, 0x011Eu, 0x0130u, 0x00D6u, 0x015Eu, 0x00DCu })
        CHECK(font->HasGlyph(cp));
    CHECK(font->GetAtlas() == 0);                             // GL context yok: yalnızca metrikler
    CHECK_NEAR(font->GetPixelHeight(), 32.0, 0.5);
    const glm::vec2 m = UI::MeasureText("Işık\nGüneş", 32.0f, font.get());
    CHECK(m.x > 40.0f && m.y > 32.0f);
}

// ---------------------------------------------------------------- Ses (cihazsız)
TEST(Audio_HeadlessPlayback) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "mme_audio_tests";
    fs::create_directories(dir);
    {   // 0.1 sn, 22050 Hz, mono 16 bit sinüs
        std::ofstream f(dir / "beep.wav", std::ios::binary);
        const uint32_t rate = 22050, n = 2205, bytes = n * 2;
        auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
        auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
        f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(1);
        u32(rate); u32(rate * 2); u16(2); u16(16); f.write("data", 4); u32(bytes);
        for (uint32_t k = 0; k < n; ++k) u16((uint16_t)(int16_t)(8000 * std::sin(k * 0.1)));
    }
    Assets::Init(dir.string());
    CHECK(Audio::Init(true));
    Audio::Play("beep.wav");
    CHECK(Audio::GetActiveVoiceCount() == 1);
    Audio::Play("yok.wav");                                   // eksik dosya: çökme yok, ses eklenmez
    CHECK(Audio::GetActiveVoiceCount() == 1);
    Audio::SetGroupVolume(SoundGroup::SFX, 0.5f);
    CHECK_NEAR(Audio::GetGroupVolume(SoundGroup::SFX), 0.5, 1e-6);
    Audio::Shutdown();
    CHECK(!Audio::IsAvailable());
    fs::remove_all(dir);
}

// ---------------------------------------------------------------- main
int main() {
    SetLogLevel(LogLevel::Warn); // test çıktısını sade tut
    int failedTests = 0;
    for (const TestCase& t : Registry()) {
        const int before = g_Failures;
        t.fn();
        const bool ok = (g_Failures == before);
        if (!ok) ++failedTests;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", t.name);
    }
    std::printf("\n%zu test, %d basarisiz\n", Registry().size(), failedTests);
    return failedTests == 0 ? 0 : 1;
}
