#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class VelocityComponent : public Component {
public:
    VelocityComponent();
    ~VelocityComponent() = default;
    
    void OnUpdate(float dt) override;
    
    glm::vec2 velocity{ 0.0f, 0.0f };
    float maxSpeed = 500.0f; // px/s
    float friction = 0.0f; // 0 = no friction, 1 = full stop
    
    void AddForce(const glm::vec2& force);
    void SetVelocity(const glm::vec2& vel);
    void ClampSpeed();
};

