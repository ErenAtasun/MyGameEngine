#include "SpriteComponent.h"
#include "Entity.h"
#include <glm/gtc/matrix_transform.hpp>

SpriteComponent::SpriteComponent() {
}

void SpriteComponent::OnRender() {
    auto entity = GetEntity();
    if (!entity || !entity->active) return;
    
    if (useUV) {
        SpriteUVDesc desc;
        desc.texture = texture;
        desc.pos = entity->position;
        desc.size = size * entity->scale;
        desc.uvMin = uvMin;
        desc.uvMax = uvMax;
        desc.tint = tint;
        desc.z = entity->zLayer;
        desc.rotation = entity->rotation;
        desc.origin = origin;
        Renderer2D::DrawSpriteUV(desc);
    } else {
        SpriteDesc desc;
        desc.texture = texture;
        desc.pos = entity->position;
        desc.size = size * entity->scale;
        desc.tint = tint;
        desc.z = entity->zLayer;
        desc.rotation = entity->rotation;
        desc.origin = origin;
        Renderer2D::DrawSprite(desc);
    }
}

