#include "CollisionSystem.h"
#include "Entity.h"
#include "VelocityComponent.h"

void CollisionSystem::Update(Scene& scene) {
    // Get all entities with colliders
    std::vector<std::pair<std::shared_ptr<Entity>, AABBCollider*>> colliders;
    
    // Note: We need access to scene's entities. For now, this will be called from Scene.
    // We'll need to add a method to Scene to get entities.
}

void CollisionSystem::ResolveCollision(
    std::shared_ptr<Entity> a, std::shared_ptr<Entity> b,
    AABBCollider* colA, AABBCollider* colB) {
    
    if (!colA || !colB) return;
    
    // Check collision
    if (!AABBCollider::CheckCollision(*colA, *colB)) return;
    
    // If both are triggers, just notify
    if (colA->isTrigger || colB->isTrigger) {
        return;
    }
    
    // Get collision normal
    glm::vec2 normal = AABBCollider::GetCollisionNormal(*colA, *colB);
    
    // Resolve collision for moving entity
    auto velA = a->GetComponent<VelocityComponent>();
    auto velB = b->GetComponent<VelocityComponent>();
    
    // If A is moving and B is static, bounce A
    if (velA && colB->isStatic) {
        // Reflect velocity
        glm::vec2 vel = velA->velocity;
        glm::vec2 reflected = vel - 2.0f * glm::dot(vel, normal) * normal;
        velA->SetVelocity(reflected);
        
        // Separate entities
        glm::vec2 overlap = normal * (colA->size.x * 0.5f + colB->size.x * 0.5f);
        a->position = b->position + overlap;
    }
    
    // If B is moving and A is static, bounce B
    if (velB && colA->isStatic) {
        glm::vec2 vel = velB->velocity;
        glm::vec2 reflected = vel - 2.0f * glm::dot(vel, -normal) * normal;
        velB->SetVelocity(reflected);
        
        glm::vec2 overlap = -normal * (colA->size.x * 0.5f + colB->size.x * 0.5f);
        b->position = a->position + overlap;
    }
}

