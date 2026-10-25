#pragma once
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    enum class BodyType {
        Static,     // hiç hareket etmez (duvar, zemin)
        Kinematic,  // kodla hareket ettirilir, çarpışmalar onu itmez (platform, raket)
        Dynamic,    // fizik tarafından hareket ettirilir ve itilir (top, oyuncu)
    };

    // Hareket verisi. Konumu Physics2D, sabit adımda velocity ile ilerletir.
    class RigidBody2D : public Component {
    public:
        BodyType type = BodyType::Dynamic;
        glm::vec2 velocity{ 0.0f, 0.0f };
        float maxSpeed = 0.0f;       // px/s, 0 = sınırsız
        float linearDamping = 0.0f;  // saniyedeki hız sönümü (0 = yok)
        float restitution = 0.0f;    // sekme: 0 = yapışır, 1 = tam esnek
        float gravityScale = 1.0f;   // Physics2D::gravity çarpanı (yalnızca Dynamic)

        void SetVelocity(const glm::vec2& v) { velocity = v; ClampSpeed(); }
        void AddImpulse(const glm::vec2& deltaV) { velocity += deltaV; ClampSpeed(); }
        void ClampSpeed();
    };
}
