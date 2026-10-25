#include <MME/ParticleEmitter.h>
#include <MME/Entity.h>

namespace mme {

    ParticleProps ParticleEmitter::MakeProps() const {
        ParticleProps p;
        p.position = GetEntity().position + offset;
        p.velocity = velocity;
        p.velocityVariation = velocityVariation;
        p.colorBegin = colorBegin;
        p.colorEnd = colorEnd;
        p.sizeBegin = sizeBegin;
        p.sizeEnd = sizeEnd;
        p.sizeVariation = sizeVariation;
        p.lifetime = lifetime;
        p.lifetimeVariation = lifetimeVariation;
        p.texture = texture;
        return p;
    }

    void ParticleEmitter::OnUpdate(float dt) {
        if (emitting && rate > 0.0f && dt > 0.0f) {
            m_Accumulator += rate * dt;
            const int count = (int)m_Accumulator;
            if (count > 0) {
                m_System.Emit(MakeProps(), count);
                m_Accumulator -= (float)count;
            }
        }
        m_System.Update(dt);
    }

    void ParticleEmitter::OnRender() {
        m_System.Render();
    }

    void ParticleEmitter::Burst(int count) {
        m_System.Emit(MakeProps(), count);
    }
}
