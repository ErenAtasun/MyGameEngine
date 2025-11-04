#include "PaddleController.h"
#include "Entity.h"
#include "VelocityComponent.h"
#include "SpriteComponent.h"
#include "Input.h"
#include <GLFW/glfw3.h>

PaddleController::PaddleController() {
}

void PaddleController::OnUpdate(float dt) {
    auto entity = GetEntity();
    if (!entity) return;
    
    auto velocity = entity->GetComponent<VelocityComponent>();
    if (!velocity) return;
    
    glm::vec2 moveDir{ 0.0f, 0.0f };
    
    if (Input::KeyDown(upKey)) {
        moveDir.y = 1.0f;
    }
    if (Input::KeyDown(downKey)) {
        moveDir.y = -1.0f;
    }
    
    // Apply movement
    if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
        velocity->SetVelocity(moveDir * moveSpeed);
    } else {
        velocity->SetVelocity({ 0.0f, 0.0f });
    }
    
    // Boundary clamping
    float halfHeight = 0.0f;
    auto sprite = entity->GetComponent<SpriteComponent>();
    if (sprite) {
        halfHeight = sprite->size.y * 0.5f;
    }
    
    if (entity->position.y - halfHeight < boundaryTop) {
        entity->position.y = boundaryTop + halfHeight;
        velocity->SetVelocity({ 0.0f, 0.0f });
    }
    if (entity->position.y + halfHeight > boundaryBottom) {
        entity->position.y = boundaryBottom - halfHeight;
        velocity->SetVelocity({ 0.0f, 0.0f });
    }
}

