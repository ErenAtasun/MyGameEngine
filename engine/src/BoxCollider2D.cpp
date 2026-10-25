#include <MME/BoxCollider2D.h>
#include <MME/Entity.h>

#include <cmath>

namespace mme {

    glm::vec2 BoxCollider2D::GetCenter() const {
        return GetEntity().position + offset;
    }

    glm::vec2 BoxCollider2D::GetHalfExtents() const {
        const glm::vec2 s = GetEntity().scale;
        return size * glm::vec2(std::abs(s.x), std::abs(s.y)) * 0.5f;
    }

    bool BoxCollider2D::Overlaps(const BoxCollider2D& other) const {
        const glm::vec2 aMin = GetMin(), aMax = GetMax();
        const glm::vec2 bMin = other.GetMin(), bMax = other.GetMax();
        return aMin.x < bMax.x && aMax.x > bMin.x &&
               aMin.y < bMax.y && aMax.y > bMin.y;
    }

    bool BoxCollider2D::Contains(const glm::vec2& p) const {
        const glm::vec2 mn = GetMin(), mx = GetMax();
        return p.x >= mn.x && p.x <= mx.x && p.y >= mn.y && p.y <= mx.y;
    }
}
