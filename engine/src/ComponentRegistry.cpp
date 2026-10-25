#include <MME/ComponentRegistry.h>
#include <MME/CameraComponent.h>
#include <MME/Collider2D.h>
#include <MME/ParticleEmitter.h>
#include <MME/ScriptComponent.h>
#include <MME/RigidBody2D.h>
#include <MME/SpriteAnimator.h>
#include <MME/SpriteComponent.h>

namespace mme {

    // ScriptInspector.cpp: script'in properties tablosunu inspector'da gösterir
    bool DrawScriptProperties(Component& component, const std::function<void()>& beforeEdit);

    namespace {
        // Fonksiyon içi statik: statik başlatma sırası sorunlarına karşı
        std::vector<ComponentInfo>& Storage() {
            static std::vector<ComponentInfo> s_Infos;
            return s_Infos;
        }
    }

    size_t ComponentRegistry::Add(ComponentInfo info) {
        auto& infos = Storage();
        for (size_t i = 0; i < infos.size(); ++i) {
            if (infos[i].name == info.name || infos[i].type == info.type) {
                infos[i] = std::move(info);
                return i;
            }
        }
        infos.push_back(std::move(info));
        return infos.size() - 1;
    }

    ComponentInfo& ComponentRegistry::At(size_t index) { return Storage()[index]; }

    const ComponentInfo* ComponentRegistry::Find(std::type_index type) {
        for (const auto& info : Storage())
            if (info.type == type) return &info;
        return nullptr;
    }

    const ComponentInfo* ComponentRegistry::Find(const std::string& name) {
        for (const auto& info : Storage())
            if (info.name == name) return &info;
        return nullptr;
    }

    const std::vector<ComponentInfo>& ComponentRegistry::GetAll() { return Storage(); }

    void ComponentRegistry::RegisterBuiltins() {
        static bool s_Done = false;
        if (s_Done) return;
        s_Done = true;

        Register<SpriteComponent>("SpriteComponent")
            .Texture("texture", &SpriteComponent::texture)
            .Field("size", &SpriteComponent::size)
            .Field("tint", &SpriteComponent::tint)
            .Field("origin", &SpriteComponent::origin, 0.01f)
            .Field("uvMin", &SpriteComponent::uvMin, 0.01f)
            .Field("uvMax", &SpriteComponent::uvMax, 0.01f);

        Register<RigidBody2D>("RigidBody2D")
            .Enum("type", &RigidBody2D::type, { "Static", "Kinematic", "Dynamic" })
            .Field("velocity", &RigidBody2D::velocity)
            .Field("angularVelocity", &RigidBody2D::angularVelocity)
            .Field("maxSpeed", &RigidBody2D::maxSpeed)
            .Field("linearDamping", &RigidBody2D::linearDamping, 0.01f)
            .Field("angularDamping", &RigidBody2D::angularDamping, 0.01f)
            .Field("gravityScale", &RigidBody2D::gravityScale, 0.01f)
            .Field("fixedRotation", &RigidBody2D::fixedRotation)
            .Field("bullet", &RigidBody2D::bullet);

        Register<BoxCollider2D>("BoxCollider2D")
            .Field("size", &BoxCollider2D::size)
            .Field("offset", &BoxCollider2D::offset)
            .Field("isTrigger", &BoxCollider2D::isTrigger)
            .Field("density", &BoxCollider2D::density, 0.01f)
            .Field("friction", &BoxCollider2D::friction, 0.01f)
            .Field("restitution", &BoxCollider2D::restitution, 0.01f);

        Register<CircleCollider2D>("CircleCollider2D")
            .Field("radius", &CircleCollider2D::radius, 0.5f)
            .Field("offset", &CircleCollider2D::offset)
            .Field("isTrigger", &CircleCollider2D::isTrigger)
            .Field("density", &CircleCollider2D::density, 0.01f)
            .Field("friction", &CircleCollider2D::friction, 0.01f)
            .Field("restitution", &CircleCollider2D::restitution, 0.01f);

        Register<CameraComponent>("CameraComponent")
            .Field("viewHeight", &CameraComponent::viewHeight)
            .Field("primary", &CameraComponent::primary)
            .Field("background", &CameraComponent::background);

        Register<ScriptComponent>("ScriptComponent")
            .Field("script", &ScriptComponent::script)
            .Field("properties", &ScriptComponent::properties).Hidden()
            .Inspector(DrawScriptProperties);

        Register<SpriteAnimator>("SpriteAnimator")
            .Field("columns", &SpriteAnimator::columns)
            .Field("rows", &SpriteAnimator::rows)
            .Field("firstFrame", &SpriteAnimator::firstFrame)
            .Field("frameCount", &SpriteAnimator::frameCount)
            .Field("fps", &SpriteAnimator::fps, 0.1f)
            .Field("speed", &SpriteAnimator::speed, 0.01f)
            .Field("loop", &SpriteAnimator::loop)
            .Field("playing", &SpriteAnimator::playing);

        Register<ParticleEmitter>("ParticleEmitter")
            .Field("emitting", &ParticleEmitter::emitting)
            .Field("rate", &ParticleEmitter::rate)
            .Field("offset", &ParticleEmitter::offset)
            .Field("velocity", &ParticleEmitter::velocity)
            .Field("velocityVariation", &ParticleEmitter::velocityVariation)
            .Field("colorBegin", &ParticleEmitter::colorBegin)
            .Field("colorEnd", &ParticleEmitter::colorEnd)
            .Field("sizeBegin", &ParticleEmitter::sizeBegin, 0.1f)
            .Field("sizeEnd", &ParticleEmitter::sizeEnd, 0.1f)
            .Field("sizeVariation", &ParticleEmitter::sizeVariation, 0.1f)
            .Field("lifetime", &ParticleEmitter::lifetime, 0.01f)
            .Field("lifetimeVariation", &ParticleEmitter::lifetimeVariation, 0.01f)
            .Texture("texture", &ParticleEmitter::texture);
    }
}
