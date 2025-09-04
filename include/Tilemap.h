#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Renderer2D.h"

struct TilemapDesc {
    GLuint atlasTex = 0;
    int atlasCols = 1, atlasRows = 1;
    int tileW = 32, tileH = 32;
    glm::vec2 originPx{ 0,0 };
    glm::vec4 tint{ 1,1,1,1 };
};

class Tilemap {
public:
    Tilemap() = default; // Varsayýlan kurucu

    bool LoadCSV(const std::string& path, const TilemapDesc& d);
    void Draw() const;

private:
    TilemapDesc desc;
    std::vector<std::vector<int>> grid;
};
