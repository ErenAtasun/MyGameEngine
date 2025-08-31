#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aTexIndex;

uniform mat4 uMVP;

out vec2 vUV;
out vec4 vColor;
flat out int vTexIndex;

void main() {
    vUV = aUV;
    vColor = aColor;
    vTexIndex = int(aTexIndex + 0.5);
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
}
