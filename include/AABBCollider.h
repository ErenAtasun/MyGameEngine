#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class AABBCollider : public Component {
public:
    AABBCollider();
    ~AABBCollider() = default;
    
    void OnUpdate(float dt) override;
    
    glm::vec2 size{ 1.0f, 1.0f };
    bool isTrigger = false; // Trigger colliders don't block movement
    bool isStatic = false; // Static colliders don't move
    
    // Get world-space bounds
    glm::vec2 GetMin() const;
    glm::vec2 GetMax() const;
    
    // Collision detection
    static bool CheckCollision(const AABBCollider& a, const AABBCollider& b);
    static glm::vec2 GetCollisionNormal(const AABBCollider& a, const AABBCollider& b);
};

