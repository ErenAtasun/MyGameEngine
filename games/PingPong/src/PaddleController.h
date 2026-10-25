#pragma once
#include <MME/MME.h>

// Klavyeyle dikey hareket. Gövde Kinematic: fizik onu itmez, hızını biz veririz.
class PaddleController : public mme::Component {
public:
    void OnFixedUpdate(float step) override;

    mme::KeyCode upKey = mme::Key::W;
    mme::KeyCode downKey = mme::Key::S;
    float moveSpeed = 480.0f;  // px/s

    // Raketin kenarlarının kalması gereken dünya aralığı (y)
    float minY = 0.0f;
    float maxY = 720.0f;
};
