#include <MME/Physics2D.h>
#include <MME/Collider2D.h>
#include <MME/Log.h>
#include <MME/RigidBody2D.h>
#include <MME/Scene.h>

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <vector>

namespace mme {

    namespace {
        b2Vec2 ToB2(const glm::vec2& v) { return { v.x, v.y }; }
        glm::vec2 ToGlm(const b2Vec2& v) { return { v.x, v.y }; }

        // b2MakeRot / b2Rot_GetAngle hızlı yaklaşımlar kullanır (~0.03° hata); her
        // senkronda biriken kaymayı önlemek için tam trigonometri
        b2Rot MakeRot(float degrees) {
            const float r = glm::radians(degrees);
            return { std::cos(r), std::sin(r) };
        }
        float RotDegrees(b2Rot q) { return glm::degrees(std::atan2(q.s, q.c)); }

        b2BodyType ToB2(BodyType t) {
            switch (t) {
                case BodyType::Static:    return b2_staticBody;
                case BodyType::Kinematic: return b2_kinematicBody;
                case BodyType::Dynamic:   return b2_dynamicBody;
            }
            return b2_staticBody;
        }

        // Shape/body userData'sında entity ID'si saklanır (işaretçi değil): entity
        // silindikten sonra gelen bir olay asla geçersiz belleğe dokunmaz.
        void* PackID(uint32_t id) { return reinterpret_cast<void*>(static_cast<uintptr_t>(id)); }
        uint32_t UnpackID(void* p) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)); }

        // Bu kadar küçük şekiller Box2D'de dejenere olur (doğrusal tolerans 0.5 px)
        constexpr float kMinExtent = 0.5f;

        void HashCombine(size_t& seed, size_t v) { seed ^= v + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2); }
        void HashFloat(size_t& seed, float f) {
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof(bits));
            HashCombine(seed, bits);
        }

        // Şekil parametrelerinin özeti: değişirse Box2D şekilleri yeniden kurulur
        size_t ShapeSignature(const Entity& e, const std::vector<const Collider2D*>& colliders) {
            size_t h = colliders.size();
            HashFloat(h, e.scale.x);
            HashFloat(h, e.scale.y);
            for (const Collider2D* c : colliders) {
                HashCombine(h, reinterpret_cast<size_t>(c));
                HashFloat(h, c->offset.x);
                HashFloat(h, c->offset.y);
                HashCombine(h, c->isTrigger);
                HashFloat(h, c->density);
                HashFloat(h, c->friction);
                HashFloat(h, c->restitution);
                if (auto* box = dynamic_cast<const BoxCollider2D*>(c)) {
                    HashFloat(h, box->size.x);
                    HashFloat(h, box->size.y);
                } else if (auto* circle = dynamic_cast<const CircleCollider2D*>(c)) {
                    HashFloat(h, circle->radius);
                }
            }
            return h;
        }

        void CollectColliders(Entity& e, std::vector<const Collider2D*>& out) {
            out.clear();
            e.ForEachComponent([&](std::type_index, Component& c) {
                if (auto* col = dynamic_cast<const Collider2D*>(&c)) out.push_back(col);
            });
        }
    }

    struct Physics2D::Impl {
        struct BodyState {
            Entity* entity = nullptr;
            b2BodyId body = b2_nullBodyId;
            size_t shapeSignature = 0;
            bool hasShapes = false;
            bool seen = false;

            // Son senkronda Box2D'ye yazılan / ondan okunan değerler: entity'deki
            // değer bunlardan farklıysa kod değiştirmiştir ve Box2D'ye aktarılır.
            glm::vec2 position{ 0.0f };
            float rotation = 0.0f;
            glm::vec2 velocity{ 0.0f };
            float angularVelocity = 0.0f;
            BodyType type = BodyType::Static;
            float linearDamping = 0.0f, angularDamping = 0.0f, gravityScale = 1.0f;
            bool fixedRotation = false, bullet = false;
        };

        b2WorldId world = b2_nullWorldId;
        std::unordered_map<uint32_t, BodyState> bodies;  // entity ID -> gövde
        std::vector<const Collider2D*> colliderScratch;

        ~Impl() { DestroyWorld(); }

        void EnsureWorld() {
            if (b2World_IsValid(world)) return;
            static bool s_UnitsSet = false;
            if (!s_UnitsSet) {
                // Box2D'nin tüm toleransları (doğrusal tolerans, uyku hızı, maksimum hız)
                // metre cinsinden; piksellerle çalışmak için bir kez ölçeklenir.
                b2SetLengthUnitsPerMeter(kPixelsPerMeter);
                s_UnitsSet = true;
            }
            b2WorldDef def = b2DefaultWorldDef();
            def.gravity = { 0.0f, 0.0f };
            world = b2CreateWorld(&def);
        }

        void DestroyWorld() {
            if (b2World_IsValid(world)) b2DestroyWorld(world);
            world = b2_nullWorldId;
            bodies.clear();
        }

        void DestroyBody(uint32_t id) {
            auto it = bodies.find(id);
            if (it == bodies.end()) return;
            if (b2Body_IsValid(it->second.body)) b2DestroyBody(it->second.body);
            bodies.erase(it);
        }

        Entity* EntityFromShape(b2ShapeId shape) const {
            if (!b2Shape_IsValid(shape)) return nullptr;
            const auto it = bodies.find(UnpackID(b2Shape_GetUserData(shape)));
            return it != bodies.end() ? it->second.entity : nullptr;
        }

        void RebuildShapes(BodyState& st, Entity& e) {
            // Mevcut şekilleri sil
            const int count = b2Body_GetShapeCount(st.body);
            if (count > 0) {
                std::vector<b2ShapeId> shapes((size_t)count);
                b2Body_GetShapes(st.body, shapes.data(), count);
                for (b2ShapeId s : shapes) b2DestroyShape(s, false);
            }

            st.hasShapes = false;
            for (const Collider2D* c : colliderScratch) {
                b2ShapeDef def = b2DefaultShapeDef();
                def.userData = PackID(e.GetID());
                def.density = std::max(c->density, 0.0f);
                def.material.friction = std::max(c->friction, 0.0f);
                def.material.restitution = std::max(c->restitution, 0.0f);
                def.isSensor = c->isTrigger;
                def.enableSensorEvents = true;
                def.enableContactEvents = true;
                def.updateBodyMass = false;  // hepsi eklendikten sonra bir kez

                if (auto* box = dynamic_cast<const BoxCollider2D*>(c)) {
                    const glm::vec2 half = box->GetHalfExtents();
                    if (half.x < kMinExtent * 0.5f || half.y < kMinExtent * 0.5f) continue;
                    const b2Polygon poly = b2MakeOffsetBox(half.x, half.y, ToB2(c->offset), b2Rot_identity);
                    b2CreatePolygonShape(st.body, &def, &poly);
                    st.hasShapes = true;
                } else if (auto* circle = dynamic_cast<const CircleCollider2D*>(c)) {
                    const float r = circle->GetWorldRadius();
                    if (r < kMinExtent * 0.5f) continue;
                    const b2Circle shape{ ToB2(c->offset), r };
                    b2CreateCircleShape(st.body, &def, &shape);
                    st.hasShapes = true;
                }
            }
            b2Body_ApplyMassFromShapes(st.body);
        }

        BodyState& CreateBody(Entity& e, RigidBody2D* rb) {
            BodyState& st = bodies[e.GetID()];
            st = BodyState{};
            st.entity = &e;

            b2BodyDef def = b2DefaultBodyDef();
            def.type = rb ? ToB2(rb->type) : b2_staticBody;
            def.position = ToB2(e.position);
            def.rotation = MakeRot(e.rotation);
            def.userData = PackID(e.GetID());
            if (rb) {
                def.linearVelocity = ToB2(rb->velocity);
                def.angularVelocity = glm::radians(rb->angularVelocity);
                def.linearDamping = rb->linearDamping;
                def.angularDamping = rb->angularDamping;
                def.gravityScale = rb->gravityScale;
                def.fixedRotation = rb->fixedRotation;
                def.isBullet = rb->bullet;
                st.type = rb->type;
                st.velocity = rb->velocity;
                st.angularVelocity = rb->angularVelocity;
                st.linearDamping = rb->linearDamping;
                st.angularDamping = rb->angularDamping;
                st.gravityScale = rb->gravityScale;
                st.fixedRotation = rb->fixedRotation;
                st.bullet = rb->bullet;
            }
            st.body = b2CreateBody(world, &def);
            st.position = e.position;
            st.rotation = e.rotation;
            st.shapeSignature = ShapeSignature(e, colliderScratch);
            RebuildShapes(st, e);
            return st;
        }

        // Koddan (ya da editörden) yapılan değişiklikleri Box2D'ye aktarır
        void SyncToBox2D(BodyState& st, Entity& e, RigidBody2D* rb) {
            const BodyType type = rb ? rb->type : BodyType::Static;
            if (type != st.type) {
                b2Body_SetType(st.body, ToB2(type));
                st.type = type;
            }
            if (e.position != st.position || e.rotation != st.rotation) {
                b2Body_SetTransform(st.body, ToB2(e.position), MakeRot(e.rotation));
                if (type != BodyType::Static) b2Body_SetAwake(st.body, true);
                st.position = e.position;
                st.rotation = e.rotation;
            }
            const size_t signature = ShapeSignature(e, colliderScratch);
            if (signature != st.shapeSignature) {
                RebuildShapes(st, e);
                st.shapeSignature = signature;
            }
            if (!rb) return;

            if (rb->velocity != st.velocity) {
                b2Body_SetLinearVelocity(st.body, ToB2(rb->velocity));
                st.velocity = rb->velocity;
            }
            if (rb->angularVelocity != st.angularVelocity) {
                b2Body_SetAngularVelocity(st.body, glm::radians(rb->angularVelocity));
                st.angularVelocity = rb->angularVelocity;
            }
            if (rb->linearDamping != st.linearDamping) b2Body_SetLinearDamping(st.body, st.linearDamping = rb->linearDamping);
            if (rb->angularDamping != st.angularDamping) b2Body_SetAngularDamping(st.body, st.angularDamping = rb->angularDamping);
            if (rb->gravityScale != st.gravityScale) b2Body_SetGravityScale(st.body, st.gravityScale = rb->gravityScale);
            if (rb->fixedRotation != st.fixedRotation) b2Body_SetFixedRotation(st.body, st.fixedRotation = rb->fixedRotation);
            if (rb->bullet != st.bullet) b2Body_SetBullet(st.body, st.bullet = rb->bullet);

            if (rb->m_PendingForce != glm::vec2(0.0f)) {
                if (type == BodyType::Dynamic) b2Body_ApplyForceToCenter(st.body, ToB2(rb->m_PendingForce), true);
                rb->m_PendingForce = { 0.0f, 0.0f };
            }
        }

        // Adımdan sonra simülasyon sonucunu entity'ye yazar
        void SyncFromBox2D(BodyState& st, Entity& e, RigidBody2D* rb) {
            if (rb) rb->m_Mass = b2Body_GetMass(st.body);
            if (st.type == BodyType::Static) return;

            e.position = ToGlm(b2Body_GetPosition(st.body));
            e.rotation = RotDegrees(b2Body_GetRotation(st.body));
            st.position = e.position;
            st.rotation = e.rotation;
            if (!rb) return;

            glm::vec2 v = ToGlm(b2Body_GetLinearVelocity(st.body));
            if (rb->maxSpeed > 0.0f && glm::length(v) > rb->maxSpeed) {
                v *= rb->maxSpeed / glm::length(v);
                b2Body_SetLinearVelocity(st.body, ToB2(v));
            }
            rb->velocity = v;
            rb->angularVelocity = glm::degrees(b2Body_GetAngularVelocity(st.body));
            st.velocity = rb->velocity;
            st.angularVelocity = rb->angularVelocity;
        }
    };

    Physics2D::Physics2D() : m_Impl(std::make_unique<Impl>()) {}
    Physics2D::~Physics2D() = default;

    void Physics2D::Clear() { m_Impl->DestroyWorld(); }

    void Physics2D::OnEntityDestroyed(Entity& entity) {
        if (b2World_IsValid(m_Impl->world)) m_Impl->DestroyBody(entity.GetID());
    }

    void Physics2D::Step(Scene& scene, float step) {
        Impl& im = *m_Impl;
        im.EnsureWorld();
        b2World_SetGravity(im.world, ToB2(gravity));

        for (auto& [id, st] : im.bodies) st.seen = false;

        // 1) Gövdeleri oluştur / güncelle
        for (auto& owned : scene.GetEntities()) {
            Entity& e = *owned;
            if (!e.IsAlive()) continue;
            auto* rb = e.GetComponent<RigidBody2D>();
            CollectColliders(e, im.colliderScratch);
            if (!rb && im.colliderScratch.empty()) continue;

            auto it = im.bodies.find(e.GetID());
            // Aynı ID'yle yeni bir entity (ör. sahne yeniden yüklendi): gövde baştan kurulur
            if (it != im.bodies.end() && it->second.entity != &e) {
                im.DestroyBody(e.GetID());
                it = im.bodies.end();
            }
            if (it == im.bodies.end()) im.CreateBody(e, rb).seen = true;
            else {
                im.SyncToBox2D(it->second, e, rb);
                it->second.seen = true;
            }
        }

        // Pasifleşen, component'i sökülen entity'lerin gövdeleri
        for (auto it = im.bodies.begin(); it != im.bodies.end();) {
            if (it->second.seen) { ++it; continue; }
            if (b2Body_IsValid(it->second.body)) b2DestroyBody(it->second.body);
            it = im.bodies.erase(it);
        }

        // 2) Simülasyon
        b2World_Step(im.world, step, std::max(1, subSteps));

        // 3) Sonuçları geri yaz
        for (auto& [id, st] : im.bodies)
            im.SyncFromBox2D(st, *st.entity, st.entity->GetComponent<RigidBody2D>());

        // 4) Olaylar: önce hepsi toplanır, sonra dağıtılır (callback'ler entity
        // silebilir/oluşturabilir; Box2D olay dizileri bir sonraki adıma kadar geçerli)
        struct Pending { Entity* a; Entity* b; glm::vec2 normal; bool trigger; };
        std::vector<Pending> events;

        const b2ContactEvents contacts = b2World_GetContactEvents(im.world);
        for (int i = 0; i < contacts.beginCount; ++i) {
            const b2ContactBeginTouchEvent& ev = contacts.beginEvents[i];
            Entity* a = im.EntityFromShape(ev.shapeIdA);
            Entity* b = im.EntityFromShape(ev.shapeIdB);
            // manifold normali A'dan B'ye bakar
            if (a && b) events.push_back({ a, b, ToGlm(ev.manifold.normal), false });
        }
        const b2SensorEvents sensors = b2World_GetSensorEvents(im.world);
        for (int i = 0; i < sensors.beginCount; ++i) {
            const b2SensorBeginTouchEvent& ev = sensors.beginEvents[i];
            Entity* a = im.EntityFromShape(ev.sensorShapeId);
            Entity* b = im.EntityFromShape(ev.visitorShapeId);
            if (a && b) events.push_back({ a, b, glm::vec2(0.0f), true });
        }

        for (const Pending& ev : events) {
            if (ev.trigger) {
                ev.a->NotifyTrigger(*ev.b);
                ev.b->NotifyTrigger(*ev.a);
            } else {
                // Collision::normal: diğer cismin yüzeyinden bu entity'ye bakar
                ev.a->NotifyCollision({ ev.b, -ev.normal });
                ev.b->NotifyCollision({ ev.a, ev.normal });
            }
        }
    }

    bool Physics2D::Raycast(const glm::vec2& from, const glm::vec2& to, RaycastHit& hit, const Entity* ignore) const {
        hit = RaycastHit{};
        if (!b2World_IsValid(m_Impl->world)) return false;

        struct Context {
            const Impl* impl;
            const Entity* ignore;
            RaycastHit* hit;
        } ctx{ m_Impl.get(), ignore, &hit };

        auto callback = [](b2ShapeId shape, b2Vec2 point, b2Vec2 normal, float fraction, void* user) -> float {
            auto& c = *static_cast<Context*>(user);
            if (b2Shape_IsSensor(shape)) return -1.0f;           // tetikler ışını durdurmaz
            Entity* e = c.impl->EntityFromShape(shape);
            if (!e || e == c.ignore) return -1.0f;
            *c.hit = { e, ToGlm(point), ToGlm(normal), fraction };
            return fraction;                                     // daha yakınını aramaya devam
        };
        b2World_CastRay(m_Impl->world, ToB2(from), ToB2(to - from), b2DefaultQueryFilter(), callback, &ctx);
        return hit.entity != nullptr;
    }

    Physics2D::Stats Physics2D::GetStats() const {
        Stats s;
        if (!b2World_IsValid(m_Impl->world)) return s;
        s.bodies = (uint32_t)m_Impl->bodies.size();
        s.awakeBodies = (uint32_t)b2World_GetAwakeBodyCount(m_Impl->world);
        s.contacts = (uint32_t)b2World_GetCounters(m_Impl->world).contactCount;
        return s;
    }
}
