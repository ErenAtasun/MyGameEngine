#include "Entity.h"
#include <algorithm>

uint32_t Entity::s_NextID = 1;

Entity::Entity(const std::string& name) : m_Name(name), m_ID(s_NextID++) {
}

Entity::~Entity() {
    m_Components.clear();
}

void Entity::OnUpdate(float dt) {
    if (!active) return;
    for (auto& comp : m_Components) {
        comp->OnUpdate(dt);
    }
}

void Entity::OnRender() {
    if (!active) return;
    for (auto& comp : m_Components) {
        comp->OnRender();
    }
}
