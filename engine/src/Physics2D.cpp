#include <MME/Physics2D.h>
#include <MME/BoxCollider2D.h>
#include <MME/RigidBody2D.h>
#include <MME/Scene.h>

#include <algorithm>
#include <cmath>

namespace mme {

    namespace {
        uint64_t PairKey(uint32_t a, uint32_t b) {
            if (a > b) std::swap(a, b);
            return ((uint64_t)a << 32) | b;
        }

        bool IsDynamic(const RigidBody2D* rb) { return rb && rb->type == BodyType::Dynamic; }
        bool IsMoving(const RigidBody2D* rb) { return rb && rb->type != BodyType::Static; }

        // Yalnızca yüzeye doğru giden hız bileşenini yansıt
        void Reflect(RigidBody2D& rb, const glm::vec2& normal) {
            const float vn = glm::dot(rb.velocity, normal);
            if (vn < 0.0f)
                rb.velocity -= (1.0f + rb.restitution) * vn * normal;
        }

        void IntegrateBody(Entity& e, RigidBody2D& rb, const glm::vec2& gravity, float dt) {
            if (rb.type == BodyType::Dynamic)
                rb.velocity += gravity * rb.gravityScale * dt;
            if (rb.linearDamping > 0.0f)
                rb.velocity /= (1.0f + rb.linearDamping * dt);
            rb.ClampSpeed();
            e.position += rb.velocity * dt;
        }

        // Izgara hücresi anahtarı (negatif koordinatlar dahil)
        int64_t CellKey(int x, int y) {
            return ((int64_t)x << 32) ^ (int64_t)(uint32_t)y;
        }

        // Bu kadar hücreyi kaplayan dev collider'lar ızgarayı şişirmesin
        constexpr int64_t kMaxCellsPerBody = 4096;
    }

    void Physics2D::Step(Scene& scene, float step) {
        // Gövdeleri topla
        m_Bodies.clear();
        m_MovingWithoutCollider.clear();
        for (auto& owned : scene.GetEntities()) {
            Entity& e = *owned;
            if (!e.IsAlive()) continue;
            auto* rb = e.GetComponent<RigidBody2D>();
            if (auto* col = e.GetComponent<BoxCollider2D>())
                m_Bodies.push_back({ &e, col, rb });
            else if (IsMoving(rb))
                m_MovingWithoutCollider.push_back(&e);
        }

        // Alt adım sayısı: hiçbir cisim bir alt adımda en küçük yarı boyutundan
        // fazla ilerlememeli; aksi halde ince bir duvarın içinden geçebilir.
        float required = 1.0f;
        for (const BodyRef& b : m_Bodies) {
            if (!IsMoving(b.body) || b.collider->isTrigger) continue;
            const glm::vec2 half = b.collider->GetHalfExtents();
            const float minHalf = std::min(half.x, half.y);
            if (minHalf <= 0.0f) continue;
            const glm::vec2 v = b.body->velocity + gravity * b.body->gravityScale * step;
            required = std::max(required, std::ceil(glm::length(v) * step / minHalf));
        }
        const int substeps = std::clamp((int)required, 1, std::max(1, maxSubsteps));
        const float dt = step / (float)substeps;

        m_Events.clear();
        m_StepContacts.clear();
        m_Reported.clear();
        m_Stats = Stats{};
        m_Stats.bodies = (uint32_t)m_Bodies.size();
        m_Stats.substeps = (uint32_t)substeps;

        for (int s = 0; s < substeps; ++s) {
            Integrate(dt);
            BuildCandidatePairs();
            ResolvePairs();
        }
        m_Contacts.swap(m_StepContacts);
        m_Stats.contacts = (uint32_t)m_Contacts.size();

        // Olaylar, tüm çözüm bittikten sonra dağıtılır: callback'ler konum/hız
        // değiştirebilir ya da entity silebilir (silme frame sonuna ertelenir).
        for (const ContactEvent& ev : m_Events) {
            if (ev.trigger) {
                ev.a->NotifyTrigger(*ev.b);
                ev.b->NotifyTrigger(*ev.a);
            } else {
                ev.a->NotifyCollision({ ev.b, ev.normal });
                ev.b->NotifyCollision({ ev.a, -ev.normal });
            }
        }
    }

    void Physics2D::Integrate(float dt) {
        for (const BodyRef& b : m_Bodies)
            if (IsMoving(b.body)) IntegrateBody(*b.entity, *b.body, gravity, dt);
        for (Entity* e : m_MovingWithoutCollider)
            IntegrateBody(*e, *e->GetComponent<RigidBody2D>(), gravity, dt);
    }

    void Physics2D::BuildCandidatePairs() {
        m_CellEntries.clear();
        m_Pairs.clear();
        const float inv = 1.0f / std::max(cellSize, 1.0f);

        std::vector<uint32_t> huge; // ızgaraya sığmayacak kadar büyükler: herkesle test edilir
        for (uint32_t i = 0; i < (uint32_t)m_Bodies.size(); ++i) {
            const glm::vec2 mn = m_Bodies[i].collider->GetMin(), mx = m_Bodies[i].collider->GetMax();
            const int x0 = (int)std::floor(mn.x * inv), x1 = (int)std::floor(mx.x * inv);
            const int y0 = (int)std::floor(mn.y * inv), y1 = (int)std::floor(mx.y * inv);
            if ((int64_t)(x1 - x0 + 1) * (y1 - y0 + 1) > kMaxCellsPerBody) { huge.push_back(i); continue; }
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x)
                    m_CellEntries.push_back({ CellKey(x, y), i });
        }

        // Hücreye göre sırala; aynı hücredekilerden çift üret
        std::sort(m_CellEntries.begin(), m_CellEntries.end());
        for (size_t start = 0; start < m_CellEntries.size();) {
            size_t end = start + 1;
            while (end < m_CellEntries.size() && m_CellEntries[end].first == m_CellEntries[start].first) ++end;
            for (size_t a = start; a < end; ++a)
                for (size_t b = a + 1; b < end; ++b) {
                    const uint32_t i = m_CellEntries[a].second, j = m_CellEntries[b].second;
                    m_Pairs.push_back(i < j ? ((uint64_t)i << 32 | j) : ((uint64_t)j << 32 | i));
                }
            start = end;
        }
        for (uint32_t h : huge)
            for (uint32_t k = 0; k < (uint32_t)m_Bodies.size(); ++k)
                if (k != h) m_Pairs.push_back(h < k ? ((uint64_t)h << 32 | k) : ((uint64_t)k << 32 | h));

        // Birden çok hücreyi paylaşan çiftler bir kez işlensin; sıralı işlem deterministik
        std::sort(m_Pairs.begin(), m_Pairs.end());
        m_Pairs.erase(std::unique(m_Pairs.begin(), m_Pairs.end()), m_Pairs.end());
        m_Stats.candidatePairs += (uint32_t)m_Pairs.size();
    }

    void Physics2D::ResolvePairs() {
        for (uint64_t pair : m_Pairs) {
            const BodyRef& A = m_Bodies[(size_t)(pair >> 32)];
            const BodyRef& B = m_Bodies[(size_t)(pair & 0xFFFFFFFFu)];
            if (!IsMoving(A.body) && !IsMoving(B.body)) continue; // iki statik asla çarpışmaz

            const glm::vec2 aMin = A.collider->GetMin(), aMax = A.collider->GetMax();
            const glm::vec2 bMin = B.collider->GetMin(), bMax = B.collider->GetMax();
            const float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
            const float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
            if (overlapX <= 0.0f || overlapY <= 0.0f) continue;

            const uint64_t key = PairKey(A.entity->GetID(), B.entity->GetID());
            m_StepContacts.insert(key);
            // Önceki adımda temas yoksa ve bu adımda henüz bildirilmediyse yeni temas
            const bool isNew = m_Contacts.count(key) == 0 && m_Reported.insert(key).second;

            if (A.collider->isTrigger || B.collider->isTrigger) {
                if (isNew) m_Events.push_back({ A.entity, B.entity, glm::vec2(0.0f), true });
                continue;
            }

            // En az iç içe geçilen eksen boyunca, B'den A'ya bakan normal
            const glm::vec2 d = A.collider->GetCenter() - B.collider->GetCenter();
            glm::vec2 normal;
            float penetration;
            if (overlapX < overlapY) { normal = { d.x >= 0.0f ? 1.0f : -1.0f, 0.0f }; penetration = overlapX; }
            else                     { normal = { 0.0f, d.y >= 0.0f ? 1.0f : -1.0f }; penetration = overlapY; }

            const bool aDyn = IsDynamic(A.body), bDyn = IsDynamic(B.body);
            if (aDyn && bDyn) {
                A.entity->position += normal * (penetration * 0.5f);
                B.entity->position -= normal * (penetration * 0.5f);
                Reflect(*A.body, normal);
                Reflect(*B.body, -normal);
            } else if (aDyn) {
                A.entity->position += normal * penetration;
                Reflect(*A.body, normal);
            } else if (bDyn) {
                B.entity->position -= normal * penetration;
                Reflect(*B.body, -normal);
            }
            // Kinematic-Kinematic / Kinematic-Static: itme yok, yalnızca olay

            if (isNew) m_Events.push_back({ A.entity, B.entity, normal, false });
        }
    }
}
