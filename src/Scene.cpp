#include "Scene.h"
#include "Entity.h"
#include <algorithm>

Scene::Scene(const std::string& name) : m_Name(name) {
}

Scene::~Scene() {
    OnDetach();
}

void Scene::OnAttach() {
    // Scene initialization
}

void Scene::OnDetach() {
    m_Entities.clear();
}

void Scene::OnUpdate(float dt) {
    for (auto& entity : m_Entities) {
        if (entity && entity->active) {
            entity->OnUpdate(dt);
        }
    }
}

void Scene::OnRender() {
    // Sort entities by z-layer for rendering
    std::vector<std::shared_ptr<Entity>> sortedEntities = m_Entities;
    std::sort(sortedEntities.begin(), sortedEntities.end(),
        [](const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b) {
            return a->zLayer < b->zLayer;
        });
    
    for (auto& entity : sortedEntities) {
        if (entity && entity->active) {
            entity->OnRender();
        }
    }
}

std::shared_ptr<Entity> Scene::CreateEntity(const std::string& name) {
    auto entity = std::make_shared<Entity>(name);
    entity->SetEntityPtr(entity); // Set self-reference for components
    m_Entities.push_back(entity);
    return entity;
}

void Scene::DestroyEntity(std::shared_ptr<Entity> entity) {
    m_Entities.erase(
        std::remove_if(m_Entities.begin(), m_Entities.end(),
            [entity](const std::shared_ptr<Entity>& e) { return e == entity; }),
        m_Entities.end());
}

