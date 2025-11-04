#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <functional>

class BallComponent : public Component {
public:
    BallComponent();
    ~BallComponent() = default;
    
    void OnUpdate(float dt) override;
    
    float speed = 300.0f;
    
    // Callbacks
    std::function<void(int side)> onGoal; // -1 = left, 1 = right
    std::function<void()> onCollision;
    
    void Reset(const glm::vec2& position);
    void Launch(const glm::vec2& direction);
    void Bounce(const glm::vec2& normal); // Public for GameScene collision
    
private:
    void CheckCollisions();
};

