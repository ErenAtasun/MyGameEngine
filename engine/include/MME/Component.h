#pragma once
#include <glm/glm.hpp>

namespace mme {

    class Entity;

    struct Collision {
        Entity* other = nullptr;
        glm::vec2 normal{ 0.0f };  // diğer cismin yüzeyinden bu entity'ye bakan normal
    };

    // Entity'ye davranış ekleyen temel sınıf. Component'in sahibi Entity'dir;
    // GetEntity() her zaman geçerlidir (component, entity'sinden önce yok olur).
    class Component {
    public:
        virtual ~Component() = default;

        virtual void OnCreate() {}                     // AddComponent'ten hemen sonra
        virtual void OnDestroy() {}                    // entity yok edilirken
        virtual void OnFixedUpdate(float /*step*/) {}  // sabit adım (fizikten önce)
        virtual void OnUpdate(float /*dt*/) {}         // frame başına
        virtual void OnRender() {}

        // Fizik olayları: yalnızca temasın başladığı adımda bir kez çağrılır
        virtual void OnCollisionEnter(const Collision& /*collision*/) {}
        virtual void OnTriggerEnter(Entity& /*other*/) {}

        Entity& GetEntity() const { return *m_Entity; }

    private:
        friend class Entity;
        Entity* m_Entity = nullptr;
    };
}
