#include <MME/ParticleSystem.h>
#include <MME/Renderer2D.h>

#include <algorithm>

namespace mme {

    ParticleSystem::ParticleSystem(uint32_t maxParticles)
        : m_Pool(std::max<uint32_t>(maxParticles, 1)) {
    }

    float ParticleSystem::Random(float variation) {
        if (variation == 0.0f) return 0.0f;
        std::uniform_real_distribution<float> dist(-variation, variation);
        return dist(m_Rng);
    }

    void ParticleSystem::Emit(const ParticleProps& props, int count) {
        for (int i = 0; i < count; ++i) {
            Particle& p = m_Pool[m_Next];
            if (!p.active) ++m_ActiveCount;   // doluysa en eskinin üstüne yazılır
            m_Next = (m_Next + 1) % (uint32_t)m_Pool.size();

            p.active = true;
            p.position = props.position;
            p.velocity = props.velocity + glm::vec2(Random(props.velocityVariation.x), Random(props.velocityVariation.y));
            p.colorBegin = props.colorBegin;
            p.colorEnd = props.colorEnd;
            p.sizeBegin = std::max(0.0f, props.sizeBegin + Random(props.sizeVariation));
            p.sizeEnd = props.sizeEnd;
            p.lifetime = std::max(0.01f, props.lifetime + Random(props.lifetimeVariation));
            p.remaining = p.lifetime;
            p.rotation = Random(180.0f);
            p.rotationSpeed = props.rotationSpeed * (Random(1.0f) < 0.0f ? -1.0f : 1.0f);
            p.texture = props.texture;
        }
    }

    void ParticleSystem::Update(float dt) {
        if (dt <= 0.0f || m_ActiveCount == 0) return;
        const float damping = 1.0f / (1.0f + drag * dt);
        for (Particle& p : m_Pool) {
            if (!p.active) continue;
            p.remaining -= dt;
            if (p.remaining <= 0.0f) {
                p.active = false;
                --m_ActiveCount;
                continue;
            }
            p.velocity = (p.velocity + gravity * dt) * damping;
            p.position += p.velocity * dt;
            p.rotation += p.rotationSpeed * dt;
        }
    }

    void ParticleSystem::Render() const {
        if (m_ActiveCount == 0) return;
        SpriteDesc s;
        for (const Particle& p : m_Pool) {
            if (!p.active) continue;
            const float t = 1.0f - p.remaining / p.lifetime; // 0 -> 1
            const float size = glm::mix(p.sizeBegin, p.sizeEnd, t);
            if (size <= 0.0f) continue;
            s.texture = p.texture;
            s.pos = p.position;
            s.size = { size, size };
            s.tint = glm::mix(p.colorBegin, p.colorEnd, t);
            s.rotation = p.rotation;
            Renderer2D::DrawSprite(s);
        }
    }

    void ParticleSystem::Clear() {
        for (Particle& p : m_Pool) p.active = false;
        m_ActiveCount = 0;
    }
}
