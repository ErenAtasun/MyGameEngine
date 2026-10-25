#include "PaddleController.h"

#include <algorithm>

void PaddleController::OnFixedUpdate(float step) {
    mme::Entity& entity = GetEntity();
    auto* body = entity.GetComponent<mme::RigidBody2D>();
    auto* collider = entity.GetComponent<mme::BoxCollider2D>();
    if (!body || !collider) return;

    float dir = 0.0f;
    if (mme::Input::KeyDown(upKey))   dir += 1.0f;
    if (mme::Input::KeyDown(downKey)) dir -= 1.0f;

    // Fizik bu adımda position += velocity * step uygulayacak; hızı, raket
    // sınırda tam duracak şekilde seç (bir adım taşıp geri çekilme olmaz).
    const float half = collider->GetHalfExtents().y;
    const float lo = minY + half;
    const float hi = std::max(lo, maxY - half);
    const float target = std::clamp(entity.position.y + dir * moveSpeed * step, lo, hi);
    body->velocity = { 0.0f, (target - entity.position.y) / step };
}
