#include "VelocityComponent.h"
#include "Entity.h"

VelocityComponent::VelocityComponent() {
}

void VelocityComponent::OnUpdate(float dt) {
    auto entity = GetEntity();
    if (!entity) return;
    
    // Apply friction
    if (friction > 0.0f) {
        velocity *= (1.0f - friction * dt);
        if (glm::length(velocity) < 10.0f) {
            velocity = { 0.0f, 0.0f };
        }
    }
    
    // Clamp speed
    ClampSpeed();
    
    // Update position
    entity->position += velocity * dt;
}

void VelocityComponent::AddForce(const glm::vec2& force) {
    velocity += force;
    ClampSpeed();
}

void VelocityComponent::SetVelocity(const glm::vec2& vel) {
    velocity = vel;
    ClampSpeed();
}

void VelocityComponent::ClampSpeed() {
    float speed = glm::length(velocity);
    if (speed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }
}

