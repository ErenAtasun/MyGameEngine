#include "AABBCollider.h"
#include "Entity.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::OnUpdate(float dt) {
    // Collision bounds update automatically with entity position
}

glm::vec2 AABBCollider::GetMin() const {
    auto entity = GetEntity();
    if (!entity) return { 0.0f, 0.0f };
    
    glm::vec2 halfSize = size * 0.5f;
    return entity->position - halfSize;
}

glm::vec2 AABBCollider::GetMax() const {
    auto entity = GetEntity();
    if (!entity) return { 0.0f, 0.0f };
    
    glm::vec2 halfSize = size * 0.5f;
    return entity->position + halfSize;
}

bool AABBCollider::CheckCollision(const AABBCollider& a, const AABBCollider& b) {
    glm::vec2 aMin = a.GetMin();
    glm::vec2 aMax = a.GetMax();
    glm::vec2 bMin = b.GetMin();
    glm::vec2 bMax = b.GetMax();
    
    return (aMin.x < bMax.x && aMax.x > bMin.x &&
            aMin.y < bMax.y && aMax.y > bMin.y);
}

glm::vec2 AABBCollider::GetCollisionNormal(const AABBCollider& a, const AABBCollider& b) {
    glm::vec2 aCenter = a.GetEntity()->position;
    glm::vec2 bCenter = b.GetEntity()->position;
    glm::vec2 diff = aCenter - bCenter;
    
    // Determine which axis has the smaller overlap
    glm::vec2 aSize = a.size;
    glm::vec2 bSize = b.size;
    glm::vec2 overlap{
        (aSize.x + bSize.x) * 0.5f - std::abs(diff.x),
        (aSize.y + bSize.y) * 0.5f - std::abs(diff.y)
    };
    
    // Return normal pointing from b to a
    if (overlap.x < overlap.y) {
        return { diff.x > 0 ? 1.0f : -1.0f, 0.0f };
    } else {
        return { 0.0f, diff.y > 0 ? 1.0f : -1.0f };
    }
}

