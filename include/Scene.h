#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "Entity.h"

class Scene {
public:
    Scene(const std::string& name = "Scene");
    virtual ~Scene();

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnUpdate(float dt);
    virtual void OnRender();
    
    std::shared_ptr<Entity> CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(std::shared_ptr<Entity> entity);
    
    const std::string& GetName() const { return m_Name; }
    
    const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return m_Entities; }
    
private:
    std::string m_Name;
    std::vector<std::shared_ptr<Entity>> m_Entities;
};

