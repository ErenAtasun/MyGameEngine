#include <MME/Entity.h>

namespace mme {

    Entity::Entity(Scene& scene, uint32_t id, std::string name)
        : m_Scene(&scene), m_ID(id), m_Name(std::move(name)) {
    }

    Entity::~Entity() {
        // Component'ler eklenme sırasının tersine sökülür
        for (auto it = m_Components.rbegin(); it != m_Components.rend(); ++it)
            it->second->OnDestroy();
        m_Components.clear();
    }

    bool Entity::RemoveComponent(std::type_index type) {
        for (auto it = m_Components.begin(); it != m_Components.end(); ++it) {
            if (it->first == type) {
                it->second->OnDestroy();
                m_Components.erase(it);
                return true;
            }
        }
        return false;
    }

    // İndeksle dolaşılıyor: bir callback içinde AddComponent çağrılırsa
    // vektör büyüyebilir ve iterator'lar geçersizleşirdi.
    void Entity::FixedUpdate(float step) {
        for (size_t i = 0; i < m_Components.size(); ++i) m_Components[i].second->OnFixedUpdate(step);
    }

    void Entity::Update(float dt) {
        for (size_t i = 0; i < m_Components.size(); ++i) m_Components[i].second->OnUpdate(dt);
    }

    void Entity::Render() {
        for (size_t i = 0; i < m_Components.size(); ++i) m_Components[i].second->OnRender();
    }

    void Entity::NotifyCollision(const Collision& c) {
        for (size_t i = 0; i < m_Components.size(); ++i) m_Components[i].second->OnCollisionEnter(c);
    }

    void Entity::NotifyTrigger(Entity& other) {
        for (size_t i = 0; i < m_Components.size(); ++i) m_Components[i].second->OnTriggerEnter(other);
    }
}
