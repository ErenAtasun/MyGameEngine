#pragma once
#include <vector>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glad/glad.h> // GLuint i�in

class Renderer2D;

/// Tek bir katman�n (grid) verisi
struct TileLayer {
    std::vector<int> cells;   // width*height, sat�r-major
    int width = 0;
    int height = 0;
    float zOrder = 0.0f;      // �izim s�ras� (k���k: altta)
    glm::vec4 tint{ 1,1,1,1 };  // iste�e ba�l� renk

    inline int at(int x, int y) const {
        return cells[y * width + x];
    }
    inline bool empty() const { return width == 0 || height == 0 || cells.empty(); }
};

/// Basit tilemap: 3 katman + atlas parametreleri
class Tilemap {
public:
    // atlasTex: OpenGL texture id, atlasCols: atlas s�tun say�s� (frame indeks hesap i�in)
    // tileW/H: ekranda tek karonun boyutu (pix)
    Tilemap(GLuint atlasTex, int atlasCols, int tileW, int tileH);

    // CSV yollar�n� y�kle (virg�l veya ';' kabul edilir). true -> ba�ar�
    bool LoadCSV_BG(const std::string& path);
    bool LoadCSV_World(const std::string& path);
    bool LoadCSV_FG(const std::string& path);

    // K�sayol: hepsini s�rayla y�kle
    bool LoadAll(const std::string& bgCsv,
        const std::string& worldCsv,
        const std::string& fgCsv);

    // �izim: origin sol-�st k��e (ekran/world koordinat�)
    void Draw(Renderer2D& r, const glm::vec2& origin);

    // Harita boyutu (d�nya katman�na g�re d�ner, yoksa bg/fg bakar)
    glm::vec2 MapPixelSize() const;

    // eri�im
    const TileLayer& BG()    const { return bg; }
    const TileLayer& World() const { return world; }
    const TileLayer& FG()    const { return fg; }

    // parametreler
    void SetLayerTint(float r, float g, float b, float a, int layerIndex); // 0:bg 1:world 2:fg
    void SetLayerZ(float z, int layerIndex);

    // atlas/ karo konfig
    void SetAtlas(GLuint tex, int cols) { atlasTex = tex; atlasCols = cols; }
    void SetTileSize(int w, int h) { tileW = w; tileH = h; }

private:
    bool loadCSV(const std::string& path, TileLayer& out,
        const char* tagForLog); // i� ayr��t�r�c�

    // H�cre -> atlas frame; -1 bo�
    inline bool isSolid(int v) const { return v >= 0; }

private:
    TileLayer bg, world, fg;
    GLuint atlasTex = 0;
    int atlasCols = 1;
    int tileW = 64, tileH = 64;
};
