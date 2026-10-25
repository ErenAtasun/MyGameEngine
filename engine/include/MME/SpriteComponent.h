#pragma once
#include <MME/Assets.h>
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    // Entity'yi dokulu ya da düz renkli bir quad olarak çizer.
    class SpriteComponent : public Component {
    public:
        void OnRender() override;

        TextureID texture = 0;              // 0 = düz renk
        glm::vec2 size{ 1.0f, 1.0f };       // entity scale ile çarpılır
        glm::vec4 tint{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec2 origin{ 0.5f, 0.5f };     // entity konumunun quad üzerindeki yeri
        glm::vec2 uvMin{ 0.0f, 0.0f };      // atlas bölgesi (varsayılan: tüm doku)
        glm::vec2 uvMax{ 1.0f, 1.0f };
    };
}
