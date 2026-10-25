#pragma once
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    // Eksen hizalı kutu (AABB) çarpıştırıcı. Boyut entity scale ile çarpılır;
    // rotasyon dikkate alınmaz.
    class BoxCollider2D : public Component {
    public:
        glm::vec2 size{ 1.0f, 1.0f };
        glm::vec2 offset{ 0.0f, 0.0f };  // entity konumuna göre merkez kayması
        bool isTrigger = false;          // true: itmez, yalnızca OnTriggerEnter üretir

        glm::vec2 GetCenter() const;
        glm::vec2 GetHalfExtents() const;
        glm::vec2 GetMin() const { return GetCenter() - GetHalfExtents(); }
        glm::vec2 GetMax() const { return GetCenter() + GetHalfExtents(); }

        bool Overlaps(const BoxCollider2D& other) const;
        bool Contains(const glm::vec2& point) const;
    };
}
