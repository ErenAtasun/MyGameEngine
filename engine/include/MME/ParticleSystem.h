#pragma once
#include <MME/Assets.h>

#include <glm/glm.hpp>
#include <cstdint>
#include <random>
#include <vector>

namespace mme {

    struct ParticleProps {
        glm::vec2 position{ 0.0f };
        glm::vec2 velocity{ 0.0f };
        glm::vec2 velocityVariation{ 0.0f };   // her eksende ± rastgele
        glm::vec4 colorBegin{ 1.0f };
        glm::vec4 colorEnd{ 1.0f, 1.0f, 1.0f, 0.0f };
        float sizeBegin = 8.0f;
        float sizeEnd = 0.0f;
        float sizeVariation = 0.0f;            // sizeBegin'e ± eklenir
        float lifetime = 1.0f;                 // saniye
        float lifetimeVariation = 0.0f;
        float rotationSpeed = 0.0f;            // derece/sn (yönü rastgele)
        TextureID texture = 0;
    };

    // Havuzlu CPU parçacık sistemi: sabit bellek, dolunca en eski parçacığın
    // üstüne yazar. Dünya uzayında yaşar; Render() bir BeginScene içinde çağrılır.
    class ParticleSystem {
    public:
        explicit ParticleSystem(uint32_t maxParticles = 2000);

        void Emit(const ParticleProps& props, int count = 1);
        void Update(float dt);
        void Render() const;
        void Clear();

        uint32_t GetActiveCount() const { return m_ActiveCount; }
        uint32_t GetCapacity() const { return (uint32_t)m_Pool.size(); }

        glm::vec2 gravity{ 0.0f };  // px/s²
        float drag = 0.0f;          // saniyedeki hız sönümü

    private:
        struct Particle {
            glm::vec2 position, velocity;
            glm::vec4 colorBegin, colorEnd;
            float sizeBegin, sizeEnd;
            float lifetime, remaining;
            float rotation, rotationSpeed;
            TextureID texture;
            bool active = false;
        };

        float Random(float variation); // [-variation, +variation]

        std::vector<Particle> m_Pool;
        uint32_t m_Next = 0;
        uint32_t m_ActiveCount = 0;
        std::mt19937 m_Rng{ 0xC0FFEEu };
    };
}
