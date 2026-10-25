#pragma once
#include <MME/Component.h>
#include <MME/Log.h>

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace mme {

    class Scene;

    // Sahnedeki bir nesne: dönüşüm (transform) + component listesi.
    // Entity'ler yalnızca Scene::CreateEntity ile oluşturulur ve sahnenin
    // sahipliğindedir; Entity& / Entity* sahne yaşadıkça geçerlidir.
    class Entity {
    public:
        Entity(Scene& scene, uint32_t id, std::string name);
        ~Entity();
        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;

        // Her tipten en fazla bir component. Dönen referans entity yaşadıkça geçerli.
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args) {
            static_assert(std::is_base_of_v<Component, T>, "T, mme::Component'ten turemeli");
            MME_ASSERT(!HasComponent<T>(), "'", m_Name, "' zaten bu component'e sahip: ", typeid(T).name());
            auto comp = std::make_unique<T>(std::forward<Args>(args)...);
            T& ref = *comp;
            static_cast<Component&>(ref).m_Entity = this;
            m_Components.emplace_back(std::type_index(typeid(T)), std::move(comp));
            ref.OnCreate();
            return ref;
        }

        template<typename T>
        T* GetComponent() {
            for (auto& [type, comp] : m_Components)
                if (type == std::type_index(typeid(T))) return static_cast<T*>(comp.get());
            return nullptr;
        }

        template<typename T>
        const T* GetComponent() const {
            for (const auto& [type, comp] : m_Components)
                if (type == std::type_index(typeid(T))) return static_cast<const T*>(comp.get());
            return nullptr;
        }

        template<typename T>
        bool HasComponent() const { return GetComponent<T>() != nullptr; }

        // Component'i hemen söker (OnDestroy çağrılır). Component'in kendi
        // callback'i içinden kendini silmek için kullanmayın.
        template<typename T>
        bool RemoveComponent() { return RemoveComponent(std::type_index(typeid(T))); }
        bool RemoveComponent(std::type_index type);

        // fn(std::type_index type, Component& component) — eklenme sırasıyla
        template<typename Fn>
        void ForEachComponent(Fn&& fn) {
            for (size_t i = 0; i < m_Components.size(); ++i)
                fn(m_Components[i].first, *m_Components[i].second);
        }
        size_t GetComponentCount() const { return m_Components.size(); }

        uint32_t GetID() const { return m_ID; }
        const std::string& GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = std::move(name); }
        Scene& GetScene() const { return *m_Scene; }

        // Frame sonunda sahneden kaldırılır; o ana kadar güncellenmez ve çizilmez.
        void Destroy() { m_PendingDestroy = true; }
        bool IsPendingDestroy() const { return m_PendingDestroy; }
        bool IsAlive() const { return active && !m_PendingDestroy; }

        // Transform
        glm::vec2 position{ 0.0f };
        float rotation = 0.0f;      // derece
        glm::vec2 scale{ 1.0f };
        float zLayer = 0.0f;        // küçük olan önce (arkada) çizilir

        bool active = true;

    private:
        friend class Scene;
        friend class Physics2D;

        void FixedUpdate(float step);
        void Update(float dt);
        void Render();
        void RenderUI();
        void NotifyCollision(const Collision& c);
        void NotifyTrigger(Entity& other);

        Scene* m_Scene;
        uint32_t m_ID;
        std::string m_Name;
        bool m_PendingDestroy = false;
        std::vector<std::pair<std::type_index, std::unique_ptr<Component>>> m_Components;
    };
}
