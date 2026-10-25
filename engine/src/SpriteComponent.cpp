#include <MME/SpriteComponent.h>
#include <MME/Entity.h>
#include <MME/Renderer2D.h>

namespace mme {

    void SpriteComponent::OnRender() {
        const Entity& entity = GetEntity();

        SpriteUVDesc desc;
        desc.texture = texture;
        desc.pos = entity.position;
        desc.size = size * entity.scale;
        desc.uvMin = uvMin;
        desc.uvMax = uvMax;
        desc.tint = tint;
        desc.rotation = entity.rotation;
        desc.origin = origin;
        Renderer2D::DrawSpriteUV(desc);
    }
}
