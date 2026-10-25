#pragma once
#include <MME/Component.h>
#include <MME/ParticleSystem.h>

namespace mme {

    // Entity'nin konumundan sürekli parçacık yayar (iz, duman, kıvılcım).
    // Parçacıklar dünya uzayında kalır; entity hareket ettikçe arkasında iz bırakır.
    class ParticleEmitter : public Component {
    public:
        explicit ParticleEmitter(uint32_t maxParticles = 512) : m_System(maxParticles) {}

        void OnUpdate(float dt) override;
        void OnRender() override;

        void Burst(int count);        // anında count tane
        ParticleSystem& GetSystem() { return m_System; }

        bool emitting = true;
        float rate = 30.0f;           // parçacık/sn
        glm::vec2 offset{ 0.0f };     // entity konumuna göre
        glm::vec2 velocity{ 0.0f };
        glm::vec2 velocityVariation{ 20.0f, 20.0f };
        glm::vec4 colorBegin{ 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };
        float sizeBegin = 8.0f;
        float sizeEnd = 0.0f;
        float sizeVariation = 0.0f;
        float lifetime = 0.6f;
        float lifetimeVariation = 0.0f;
        TextureID texture = 0;

    private:
        ParticleProps MakeProps() const;

        ParticleSystem m_System;
        float m_Accumulator = 0.0f;
    };
}
