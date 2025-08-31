#version 330 core
in vec2 vUV;
in vec4 vColor;
flat in int vTexIndex;

out vec4 FragColor;

uniform sampler2D uTextures[8];

vec4 SampleFromIndex(int i, vec2 uv) {
    if(i==0) return texture(uTextures[0], uv);
    if(i==1) return texture(uTextures[1], uv);
    if(i==2) return texture(uTextures[2], uv);
    if(i==3) return texture(uTextures[3], uv);
    if(i==4) return texture(uTextures[4], uv);
    if(i==5) return texture(uTextures[5], uv);
    if(i==6) return texture(uTextures[6], uv);
    return texture(uTextures[7], uv);
}

void main() {
    vec4 tex = SampleFromIndex(vTexIndex, vUV);
    FragColor = tex * vColor;
}
