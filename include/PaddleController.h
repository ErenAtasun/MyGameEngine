#pragma once
#include "Component.h"

class PaddleController : public Component {
public:
    PaddleController();
    ~PaddleController() = default;
    
    void OnUpdate(float dt) override;
    
    // Controls: stores key codes (set from outside)
    int upKey = 0;
    int downKey = 0;
    
    float moveSpeed = 400.0f; // px/s
    float boundaryTop = 0.0f;    // World space boundaries
    float boundaryBottom = 720.0f;
};

