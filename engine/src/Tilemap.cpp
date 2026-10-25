#include <MME/Tilemap.h>
#include <MME/Log.h>
#include <MME/OrthoCamera.h>
#include <MME/Renderer2D.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mme {

    bool Tilemap::LoadCSV(const std::string& relativePath, const TilemapDesc& desc) {
        m_Desc = desc;
        m_Grid.clear();

        const std::string path = Assets::GetPath(relativePath);
        std::ifstream f(path);
        if (!f.is_open()) {
            MME_ERROR("[Tilemap] Dosya acilamadi: ", path);
            return false;
        }

        size_t width = 0;
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
            if (row.empty()) continue;
            width = std::max(width, row.size());
            m_Grid.push_back(std::move(row));
        }
        // Eksik hücreli satırları boşlukla tamamla: Draw dikdörtgen ızgara varsayar
        for (auto& row : m_Grid) row.resize(width, -1);

        MME_INFO("[Tilemap] Yuklendi: ", relativePath, " (", width, "x", m_Grid.size(), ")");
        return true;
    }

    int Tilemap::GetTile(int col, int row) const {
        if (row < 0 || row >= GetRows() || col < 0 || col >= GetColumns()) return -1;
        return m_Grid[(size_t)row][(size_t)col];
    }

    void Tilemap::SetTile(int col, int row, int id) {
        if (row < 0 || row >= GetRows() || col < 0 || col >= GetColumns()) return;
        m_Grid[(size_t)row][(size_t)col] = id;
    }

    bool Tilemap::WorldToTile(const glm::vec2& world, int& col, int& row) const {
        const glm::vec2 local = world - m_Desc.origin;
        col = (int)std::floor(local.x / (float)m_Desc.tileW);
        const int rowFromBottom = (int)std::floor(local.y / (float)m_Desc.tileH);
        row = GetRows() - 1 - rowFromBottom;
        return col >= 0 && col < GetColumns() && row >= 0 && row < GetRows();
    }

    glm::vec2 Tilemap::TileToWorld(int col, int row) const {
        return m_Desc.origin + glm::vec2((float)(col * m_Desc.tileW), (float)((GetRows() - 1 - row) * m_Desc.tileH));
    }

    void Tilemap::Draw() const {
        DrawRange(0, GetColumns(), 0, GetRows());
    }

    void Tilemap::Draw(const OrthoCamera& camera) const {
        if (m_Grid.empty()) return;

        // Görünen dünya dikdörtgeni (rotasyon yok sayılır)
        const glm::vec2 half = camera.GetVisibleSize() * 0.5f;
        const glm::vec2 camPos = camera.GetPosition();
        const glm::vec2 minW = camPos - half - m_Desc.origin;
        const glm::vec2 maxW = camPos + half - m_Desc.origin;

        const int rows = GetRows();
        const int col0 = std::max(0, (int)std::floor(minW.x / (float)m_Desc.tileW));
        const int col1 = std::min(GetColumns(), (int)std::ceil(maxW.x / (float)m_Desc.tileW));
        // Dünya y'si yukarı, satır indeksi aşağı doğru artar
        const int row0 = std::max(0, rows - (int)std::ceil(maxW.y / (float)m_Desc.tileH));
        const int row1 = std::min(rows, rows - (int)std::floor(minW.y / (float)m_Desc.tileH));

        DrawRange(col0, col1, row0, row1);
    }

    void Tilemap::DrawRange(int col0, int col1, int row0, int row1) const {
        if (!m_Desc.atlasTex || col0 >= col1 || row0 >= row1) return;

        const int rows = GetRows();

        SpriteUVDesc s;
        s.texture = m_Desc.atlasTex;
        s.size = { (float)m_Desc.tileW, (float)m_Desc.tileH };
        s.tint = m_Desc.tint;
        s.origin = { 0.0f, 0.0f }; // pos = karonun sol-alt köşesi

        for (int y = row0; y < row1; ++y) {
            const float worldY = m_Desc.origin.y + (float)((rows - 1 - y) * m_Desc.tileH);
            for (int x = col0; x < col1; ++x) {
                const int id = m_Grid[(size_t)y][(size_t)x];
                if (id < 0) continue;

                GridCellUV(m_Desc.atlasCols, m_Desc.atlasRows, id, s.uvMin, s.uvMax);
                s.pos = { m_Desc.origin.x + (float)(x * m_Desc.tileW), worldY };
                Renderer2D::DrawSpriteUV(s);
            }
        }
    }
}
