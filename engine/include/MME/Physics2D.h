#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>

namespace mme {

    class Scene;
    class Entity;

    // Box2D tabanlı 2D fizik. Birimler piksel: Box2D'nin toleransları
    // kPixelsPerMeter ile ölçeklenir (100 px = 1 m), yani 32-200 px'lik nesneler
    // en kararlı aralıktadır.
    //
    // Component'ler veri olarak kalır: her sabit adımda
    //   1) RigidBody2D / Collider2D taşıyan entity'ler için Box2D gövdeleri
    //      oluşturulur ya da güncellenir (koddan değişen konum/hız/şekil aktarılır),
    //   2) Box2D adımı çalışır,
    //   3) konum, rotasyon ve hız entity'lere geri yazılır,
    //   4) yeni başlayan temaslar için OnCollisionEnter / OnTriggerEnter çağrılır.
    // Static/Kinematic çiftleri kendi aralarında çarpışmaz (en az biri Dynamic olmalı).
    class Physics2D {
    public:
        static constexpr float kPixelsPerMeter = 100.0f;

        Physics2D();
        ~Physics2D();
        Physics2D(const Physics2D&) = delete;
        Physics2D& operator=(const Physics2D&) = delete;

        void Step(Scene& scene, float step);
        void Clear();                          // tüm gövdeleri kaldırır (Scene::Clear çağırır)
        void OnEntityDestroyed(Entity& entity); // Scene, entity'yi silmeden hemen önce çağırır

        glm::vec2 gravity{ 0.0f, 0.0f };       // px/s², ör. platform oyunu için { 0, -1500 }
        int subSteps = 4;                      // Box2D çözücü alt adımı (kararlılık / maliyet)

        struct RaycastHit {
            Entity* entity = nullptr;
            glm::vec2 point{ 0.0f };
            glm::vec2 normal{ 0.0f };
            float fraction = 0.0f;             // 0 = from, 1 = to
        };
        // from'dan to'ya en yakın katı (tetik olmayan) çarpıştırıcı. ignore verilirse o
        // entity atlanır (ör. oyuncunun kendisi). Son adımdaki konumlara göre çalışır.
        bool Raycast(const glm::vec2& from, const glm::vec2& to, RaycastHit& hit, const Entity* ignore = nullptr) const;

        struct Stats {
            uint32_t bodies = 0;        // fizik gövdesi olan entity sayısı
            uint32_t awakeBodies = 0;   // uyumayan (simüle edilen) gövdeler
            uint32_t contacts = 0;      // şu an temas eden şekil çiftleri
        };
        Stats GetStats() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
