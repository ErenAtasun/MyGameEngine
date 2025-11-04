#include "BallComponent.h"
#include "Entity.h"
#include "VelocityComponent.h"
#include "AABBCollider.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

BallComponent::BallComponent() {
}

void BallComponent::OnUpdate(float dt) {
    CheckCollisions();
}

void BallComponent::CheckCollisions() {
    auto ballEntity = GetEntity();
    if (!ballEntity) return;
    
    auto ballCollider = ballEntity->GetComponent<AABBCollider>();
    auto ballVelocity = ballEntity->GetComponent<VelocityComponent>();
    if (!ballCollider || !ballVelocity) return;
    
    // Get world boundaries (will be passed from GameScene)
    float worldTop = 0.0f;
    float worldBottom = 720.0f;
    float worldLeft = 0.0f;
    float worldRight = 1280.0f;
    
    // Boundary collisions (top/bottom walls)
    glm::vec2 min = ballCollider->GetMin();
    glm::vec2 max = ballCollider->GetMax();
    
    // Top/Bottom bounce
    if (min.y <= worldTop) {
        Bounce({ 0.0f, 1.0f });
        ballEntity->position.y = worldTop + ballCollider->size.y * 0.5f;
    }
    if (max.y >= worldBottom) {
        Bounce({ 0.0f, -1.0f });
        ballEntity->position.y = worldBottom - ballCollider->size.y * 0.5f;
    }
    
    // Left/Right goals
    if (min.x <= worldLeft) {
        if (onGoal) onGoal(-1); // Left goal
        Reset({ worldRight * 0.5f, worldBottom * 0.5f });
    }
    if (max.x >= worldRight) {
        if (onGoal) onGoal(1); // Right goal
        Reset({ worldRight * 0.5f, worldBottom * 0.5f });
    }
    
    // Paddle collisions (checked via Scene in GameScene::OnUpdate)
}

void BallComponent::Bounce(const glm::vec2& normal) {
    auto entity = GetEntity();
    if (!entity) return;
    
    auto velocity = entity->GetComponent<VelocityComponent>();
    if (!velocity) return;
    
    // Reflect velocity
    glm::vec2 vel = velocity->velocity;
    float dot = glm::dot(vel, normal);
    glm::vec2 reflected = vel - 2.0f * dot * normal;
    
    // Maintain speed
    float currentSpeed = glm::length(vel);
    if (currentSpeed > 0.0f) {
        float len = glm::length(reflected);
        if (len > 0.0f) {
            reflected = (reflected / len) * currentSpeed;
        } else {
            reflected = normal * speed;
        }
    } else {
        float len = glm::length(reflected);
        if (len > 0.0f) {
            reflected = (reflected / len) * speed;
        } else {
            reflected = normal * speed;
        }
    }
    
    velocity->SetVelocity(reflected);
    
    if (onCollision) onCollision();
}

void BallComponent::Reset(const glm::vec2& position) {
    auto entity = GetEntity();
    if (!entity) return;
    
    auto velocity = entity->GetComponent<VelocityComponent>();
    if (!velocity) return;
    
    entity->position = position;
    velocity->SetVelocity({ 0.0f, 0.0f });
}

void BallComponent::Launch(const glm::vec2& direction) {
    auto entity = GetEntity();
    if (!entity) return;
    
    auto velocity = entity->GetComponent<VelocityComponent>();
    if (!velocity) return;
    
    glm::vec2 dir = glm::normalize(direction);
    velocity->SetVelocity(dir * speed);
}

