#include <MME/RigidBody2D.h>

namespace mme {

    void RigidBody2D::ClampSpeed() {
        if (maxSpeed <= 0.0f) return;
        const float speed = glm::length(velocity);
        if (speed > maxSpeed)
            velocity *= maxSpeed / speed;
    }
}
