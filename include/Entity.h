#pragma once
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Component.h"

class Entity {
public:
    Entity(const std::string& name = "Entity");
    ~Entity();
    
    void OnUpdate(float dt);
    void OnRender();
    
    void SetEntityPtr(std::shared_ptr<Entity> ptr) { m_EntityPtr = ptr; }
    
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        auto comp = std::make_shared<T>(std::forward<Args>(args)...);
        if (m_EntityPtr) {
            comp->SetEntity(m_EntityPtr);
        }
        m_Components.push_back(comp);
        return comp;
    }
    
    template<typename T>
    std::shared_ptr<T> GetComponent() {
        for (auto& comp : m_Components) {
            if (auto found = std::dynamic_pointer_cast<T>(comp)) {
                return found;
            }
        }
        return nullptr;
    }
    
    template<typename T>
    bool HasComponent() {
        return GetComponent<T>() != nullptr;
    }
    
    const std::string& GetName() const { return m_Name; }
    uint32_t GetID() const { return m_ID; }
    
    // Transform
    glm::vec2 position{ 0.0f };
    float rotation = 0.0f;
    glm::vec2 scale{ 1.0f };
    float zLayer = 0.0f;
    
    bool active = true;
    
private:
    std::string m_Name;
    uint32_t m_ID;
    std::vector<std::shared_ptr<Component>> m_Components;
    std::shared_ptr<Entity> m_EntityPtr; // For component entity reference
    
    static uint32_t s_NextID;
};

