#pragma once
#include <MME/MME.h>
#include <functional>

// Kale alanını işaretler (trigger collider ile birlikte kullanılır)
class Goal : public mme::Component {
public:
    int side = 0; // -1 = sol kale, 1 = sağ kale
};

// Pong topu: raketin neresine çarptıysa o açıyla seker, her vuruşta hızlanır.
// Duvar sekmeleri fizik sisteminden gelir (collider restitution = 1).
class BallComponent : public mme::Component {
public:
    void OnCollisionEnter(const mme::Collision& collision) override;
    void OnTriggerEnter(mme::Entity& other) override;

    void Launch(const glm::vec2& direction); // baseSpeed ile
    void Stop();
    float GetSpeed() const;

    float baseSpeed = 380.0f;        // px/s
    float speedGain = 1.06f;         // raket vuruşu başına çarpan
    float maxSpeed = 950.0f;
    float maxBounceAngleDeg = 55.0f; // raketin ucuna vurunca

    std::function<void(int side)> onGoal;   // girilen kale: -1 sol, 1 sağ
    // Görsel/işitsel geri bildirim için (temas noktası, yüzey normali, yeni hız)
    std::function<void(glm::vec2 point, glm::vec2 normal, float speed)> onPaddleHit;
    std::function<void(glm::vec2 point, glm::vec2 normal)> onWallHit;
};
