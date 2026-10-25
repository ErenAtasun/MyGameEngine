#include <MME/Collider2D.h>
#include <MME/Entity.h>

#include <algorithm>
#include <cmath>

namespace mme {

    namespace {
        glm::vec2 Rotate(const glm::vec2& v, float degrees) {
            const float r = glm::radians(degrees);
            const float c = std::cos(r), s = std::sin(r);
            return { c * v.x - s * v.y, s * v.x + c * v.y };
        }
    }

    glm::vec2 Collider2D::GetCenter() const {
        const Entity& e = GetEntity();
        return e.position + Rotate(offset, e.rotation);
    }

    glm::vec2 BoxCollider2D::GetHalfExtents() const {
        const glm::vec2 s = GetEntity().scale;
        return size * glm::vec2(std::abs(s.x), std::abs(s.y)) * 0.5f;
    }

    bool BoxCollider2D::Contains(const glm::vec2& p) const {
        // Noktayı kutunun yerel uzayına döndür
        const glm::vec2 local = Rotate(p - GetCenter(), -GetEntity().rotation);
        const glm::vec2 half = GetHalfExtents();
        return std::abs(local.x) <= half.x && std::abs(local.y) <= half.y;
    }

    void BoxCollider2D::GetBounds(glm::vec2& mn, glm::vec2& mx) const {
        const glm::vec2 half = GetHalfExtents();
        const float r = glm::radians(GetEntity().rotation);
        const float c = std::abs(std::cos(r)), s = std::abs(std::sin(r));
        const glm::vec2 ext{ c * half.x + s * half.y, s * half.x + c * half.y };
        const glm::vec2 center = GetCenter();
        mn = center - ext;
        mx = center + ext;
    }

    float CircleCollider2D::GetWorldRadius() const {
        const glm::vec2 s = GetEntity().scale;
        return radius * std::max(std::abs(s.x), std::abs(s.y));
    }

    bool CircleCollider2D::Contains(const glm::vec2& p) const {
        const glm::vec2 d = p - GetCenter();
        const float r = GetWorldRadius();
        return glm::dot(d, d) <= r * r;
    }

    void CircleCollider2D::GetBounds(glm::vec2& mn, glm::vec2& mx) const {
        const glm::vec2 c = GetCenter();
        const float r = GetWorldRadius();
        mn = c - glm::vec2(r);
        mx = c + glm::vec2(r);
    }
}
