#pragma once
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    enum class BodyType {
        Static,     // hiç hareket etmez (duvar, zemin)
        Kinematic,  // kodla hareket ettirilir, çarpışmalar onu itmez (platform, raket)
        Dynamic,    // fizik tarafından hareket ettirilir ve itilir (top, oyuncu)
    };

    // Hareket verisi. Physics2D her sabit adımda bu alanları fizik dünyasına
    // aktarır, adımdan sonra konum/rotasyon/hızı geri yazar. Alanları doğrudan
    // değiştirmek (ör. velocity = ...) bir sonraki adımda uygulanır.
    // Malzeme (sürtünme, sekme, yoğunluk) çarpıştırıcıdadır: Collider2D.
    class RigidBody2D : public Component {
    public:
        BodyType type = BodyType::Dynamic;
        glm::vec2 velocity{ 0.0f, 0.0f };   // px/s
        float angularVelocity = 0.0f;       // derece/s (saat yönünün tersi +)
        float maxSpeed = 0.0f;              // px/s, 0 = sınırsız
        float linearDamping = 0.0f;         // saniyedeki hız sönümü (0 = yok)
        float angularDamping = 0.0f;
        float gravityScale = 1.0f;          // Physics2D::gravity çarpanı (yalnızca Dynamic)
        bool fixedRotation = false;         // karakterler için: çarpışmalar döndürmez
        bool bullet = false;                // çok hızlı küçük cisimler için sürekli çarpışma

        void SetVelocity(const glm::vec2& v) { velocity = v; ClampSpeed(); }
        // Hız değişimi (kütleden bağımsız): velocity += deltaV
        void AddImpulse(const glm::vec2& deltaV) { velocity += deltaV; ClampSpeed(); }
        // Kuvvet (kütleye bağlı), yalnızca bir sonraki fizik adımında uygulanır
        void AddForce(const glm::vec2& force) { m_PendingForce += force; }
        void ClampSpeed();

        // Fizik dünyasındaki kütle (çarpıştırıcıların yoğunluk x alanı). Gövde
        // henüz oluşturulmadıysa (ilk adımdan önce) 0.
        float GetMass() const { return m_Mass; }

    private:
        friend class Physics2D;
        glm::vec2 m_PendingForce{ 0.0f, 0.0f };
        float m_Mass = 0.0f;
    };
}
