#pragma once
#include "Component.h"
#include "Renderer2D.h"
#include <glad/glad.h>

class SpriteComponent : public Component {
public:
    SpriteComponent();
    ~SpriteComponent() = default;
    
    void OnRender() override;
    
    GLuint texture = 0;
    glm::vec2 size{ 1.0f, 1.0f };
    glm::vec4 tint{ 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec2 origin{ 0.5f, 0.5f };
    bool useUV = false;
    glm::vec2 uvMin{ 0.0f, 0.0f };
    glm::vec2 uvMax{ 1.0f, 1.0f };
};

