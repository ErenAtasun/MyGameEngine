#pragma once
#include <memory>

class Entity;

class Component {
public:
    virtual ~Component() = default;
    virtual void OnUpdate(float dt) {}
    virtual void OnRender() {}
    
    std::shared_ptr<Entity> GetEntity() { return m_Entity.lock(); }
    std::shared_ptr<Entity> GetEntity() const { return m_Entity.lock(); }
    void SetEntity(std::shared_ptr<Entity> entity) { m_Entity = entity; }
    
protected:
    std::weak_ptr<Entity> m_Entity;
};

