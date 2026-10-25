#pragma once
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    // Çarpıştırıcıların ortak kısmı: yüzey malzemesi ve tetik ayarı.
    // Şekil entity'nin konumu, rotasyonu ve ölçeğiyle birlikte döner/büyür.
    class Collider2D : public Component {
    public:
        glm::vec2 offset{ 0.0f, 0.0f };  // entity'ye göre yerel kayma (entity ile birlikte döner)
        bool isTrigger = false;          // true: itmez, yalnızca OnTriggerEnter üretir
        float density = 1.0f;            // kütle = yoğunluk x alan (piksel²)
        float friction = 0.4f;           // 0 = buz, 1 = zımpara
        float restitution = 0.0f;        // sekme: 0 = yapışır, 1 = tam esnek

        // Şeklin dünya uzayındaki merkezi (offset entity rotasyonuyla döndürülür)
        glm::vec2 GetCenter() const;

        virtual bool Contains(const glm::vec2& worldPoint) const = 0;
        // Dönmüş şekli içine alan eksen hizalı kutu
        virtual void GetBounds(glm::vec2& min, glm::vec2& max) const = 0;
    };

    // Dikdörtgen çarpıştırıcı. Boyut entity scale ile çarpılır, entity rotasyonuyla döner.
    class BoxCollider2D : public Collider2D {
    public:
        glm::vec2 size{ 1.0f, 1.0f };

        glm::vec2 GetHalfExtents() const;   // ölçeklenmiş yarı boyut (dönmemiş)
        bool Contains(const glm::vec2& worldPoint) const override;
        void GetBounds(glm::vec2& min, glm::vec2& max) const override;
    };

    // Daire çarpıştırıcı. Yarıçap, entity ölçeğinin büyük ekseniyle çarpılır.
    class CircleCollider2D : public Collider2D {
    public:
        float radius = 0.5f;

        float GetWorldRadius() const;
        bool Contains(const glm::vec2& worldPoint) const override;
        void GetBounds(glm::vec2& min, glm::vec2& max) const override;
    };
}
