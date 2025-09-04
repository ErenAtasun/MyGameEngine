#include "Tilemap.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Tilemap::LoadCSV(const std::string& path, const TilemapDesc& d) {
    desc = d;
    grid.clear();

    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[Tilemap] Dosya acilamadi: " << path << "\n";
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        std::vector<int> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            try {
                row.push_back(std::stoi(cell));
            }
            catch (...) {
                row.push_back(-1);
            }
        }
        if (!row.empty())
            grid.push_back(row);
    }

    std::cout << "[Tilemap] Yuklendi: " << path
        << " (" << (grid.empty() ? 0 : grid[0].size())
        << "x" << grid.size() << ")\n";
    return true;
}

void Tilemap::Draw() const {
    if (!desc.atlasTex) return;

    for (size_t y = 0; y < grid.size(); ++y) {
        for (size_t x = 0; x < grid[y].size(); ++x) {
            int id = grid[y][x];
            if (id < 0) continue;

            int col = id % desc.atlasCols;
            int row = id / desc.atlasCols;

            glm::vec2 uvMin, uvMax;
            float u0 = (float)col / desc.atlasCols;
            float v0 = (float)row / desc.atlasRows;
            float u1 = (float)(col + 1) / desc.atlasCols;
            float v1 = (float)(row + 1) / desc.atlasRows;
            uvMin = { u0, v0 };
            uvMax = { u1, v1 };

            SpriteUVDesc s;
            s.texture = desc.atlasTex;
            s.pos = { desc.originPx.x + (float)x * desc.tileW,
                      desc.originPx.y + (float)y * desc.tileH };
            s.size = { (float)desc.tileW, (float)desc.tileH };
            s.uvMin = uvMin;
            s.uvMax = uvMax;
            s.tint = desc.tint;

            Renderer2D::DrawSpriteUV(s);
        }
    }
}
