#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace mme {

    class Scene;
    class Entity;
    class BoxCollider2D;
    class RigidBody2D;

    // 2D AABB fizik. Her sabit adımda:
    //   1) hızlı cisimler varsa adım alt adımlara bölünür (tünellemeyi önler),
    //   2) RigidBody2D hızları uygulanır (yerçekimi, sönüm, hız sınırı),
    //   3) uzamsal ızgara (broadphase) aday çiftleri bulur, çakışanlar çözülür,
    //   4) yeni başlayan temaslar için OnCollisionEnter / OnTriggerEnter çağrılır.
    class Physics2D {
    public:
        void Step(Scene& scene, float step);

        glm::vec2 gravity{ 0.0f, 0.0f }; // px/s², ör. platform oyunu için { 0, -980 }
        float cellSize = 128.0f;         // broadphase hücre boyutu (tipik nesneden büyük olmalı)
        int maxSubsteps = 8;

        struct Stats {
            uint32_t bodies = 0;          // collider sayısı
            uint32_t candidatePairs = 0;  // broadphase'in önerdiği çiftler (alt adımlar toplamı)
            uint32_t contacts = 0;        // şu an temas eden çiftler
            uint32_t substeps = 0;        // son adımdaki alt adım sayısı
        };
        const Stats& GetStats() const { return m_Stats; }

    private:
        struct BodyRef {
            Entity* entity;
            BoxCollider2D* collider;
            RigidBody2D* body;   // yoksa statik kabul edilir
        };
        struct ContactEvent {
            Entity* a;
            Entity* b;
            glm::vec2 normal;    // b'den a'ya
            bool trigger;
        };

        void Integrate(float dt);
        void BuildCandidatePairs();
        void ResolvePairs();

        std::vector<BodyRef> m_Bodies;
        std::vector<Entity*> m_MovingWithoutCollider;
        std::vector<std::pair<int64_t, uint32_t>> m_CellEntries; // (hücre, gövde)
        std::vector<uint64_t> m_Pairs;                           // (i << 32 | j), i < j
        std::vector<ContactEvent> m_Events;
        std::unordered_set<uint64_t> m_Contacts;     // önceki adımda temas eden çiftler
        std::unordered_set<uint64_t> m_StepContacts; // bu adımda temas edenler
        std::unordered_set<uint64_t> m_Reported;     // bu adımda olayı üretilmiş çiftler
        Stats m_Stats;
    };
}
