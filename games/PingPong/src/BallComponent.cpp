#include "BallComponent.h"
#include "PaddleController.h"

#include <algorithm>
#include <cmath>

void BallComponent::OnCollisionEnter(const mme::Collision& collision) {
    mme::Entity& other = *collision.other;
    auto* body = GetEntity().GetComponent<mme::RigidBody2D>();
    auto* collider = GetEntity().GetComponent<mme::BoxCollider2D>();
    if (!body || !collider) return;

    // Temas noktası: topun normal yönündeki kenarı
    const glm::vec2 point = GetEntity().position - collision.normal * collider->GetHalfExtents();

    // Yalnızca raketin ön yüzü: üst/alt kenara sürtünme fiziğin sekmesiyle kalır
    if (!other.HasComponent<PaddleController>() || std::abs(collision.normal.x) < 0.5f) {
        if (onWallHit) onWallHit(point, collision.normal);
        return;
    }

    auto* paddleCollider = other.GetComponent<mme::BoxCollider2D>();
    if (!paddleCollider) return;

    // -1 (alt uç) .. 1 (üst uç): raketin neresine çarptı
    const float halfH = paddleCollider->GetHalfExtents().y;
    const float hit = std::clamp((GetEntity().position.y - other.position.y) / halfH, -1.0f, 1.0f);
    const float angle = glm::radians(hit * maxBounceAngleDeg);
    const float speed = std::min(glm::length(body->velocity) * speedGain, maxSpeed);

    // normal.x raketten topa bakar: top her zaman raketten uzaklaşır
    body->velocity = glm::vec2(std::cos(angle) * collision.normal.x, std::sin(angle)) * speed;
    if (onPaddleHit) onPaddleHit(point, collision.normal, speed);
}

void BallComponent::OnTriggerEnter(mme::Entity& other) {
    if (auto* goal = other.GetComponent<Goal>(); goal && onGoal)
        onGoal(goal->side);
}

void BallComponent::Launch(const glm::vec2& direction) {
    if (auto* body = GetEntity().GetComponent<mme::RigidBody2D>())
        body->velocity = glm::normalize(direction) * baseSpeed;
}

void BallComponent::Stop() {
    if (auto* body = GetEntity().GetComponent<mme::RigidBody2D>())
        body->velocity = { 0.0f, 0.0f };
}

float BallComponent::GetSpeed() const {
    const auto* body = GetEntity().GetComponent<mme::RigidBody2D>();
    return body ? glm::length(body->velocity) : 0.0f;
}
