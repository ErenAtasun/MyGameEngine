#pragma once
#include <MME/Assets.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace mme {

    class OrthoCamera;

    struct TilemapDesc {
        TextureID atlasTex = 0;
        int atlasCols = 1, atlasRows = 1;   // atlastaki karo ızgarası
        int tileW = 32, tileH = 32;         // dünyadaki karo boyutu (px)
        glm::vec2 origin{ 0, 0 };           // haritanın sol-alt köşesi (dünya)
        glm::vec4 tint{ 1, 1, 1, 1 };
    };

    // CSV karo haritası. CSV'nin ilk satırı haritanın en üstü, karo kimliği
    // atlası soldan sağa / yukarıdan aşağı sayar (0 = sol-üst). -1 = boş.
    class Tilemap {
    public:
        Tilemap() = default;

        bool LoadCSV(const std::string& relativePath, const TilemapDesc& desc); // assets klasörüne göre
        void SetDesc(const TilemapDesc& desc) { m_Desc = desc; }

        void Draw() const;                          // tüm harita
        void Draw(const OrthoCamera& camera) const; // yalnızca görünen karolar

        int GetColumns() const { return m_Grid.empty() ? 0 : (int)m_Grid[0].size(); }
        int GetRows() const { return (int)m_Grid.size(); }
        int GetTile(int col, int row) const;        // row 0 = en üst satır
        void SetTile(int col, int row, int id);

        // Dünya noktası hangi karoda? Harita dışındaysa false.
        bool WorldToTile(const glm::vec2& world, int& col, int& row) const;
        glm::vec2 TileToWorld(int col, int row) const; // karonun sol-alt köşesi
        glm::vec2 GetTileSize() const { return { (float)m_Desc.tileW, (float)m_Desc.tileH }; }
        glm::vec2 GetWorldSize() const { return { (float)(GetColumns() * m_Desc.tileW), (float)(GetRows() * m_Desc.tileH) }; }

    private:
        void DrawRange(int col0, int col1, int row0, int row1) const;

        TilemapDesc m_Desc;
        std::vector<std::vector<int>> m_Grid;
    };
}
