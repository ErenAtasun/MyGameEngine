#pragma once
#include <vector>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glad/glad.h> // GLuint için

class Renderer2D;

/// Tek bir katmanýn (grid) verisi
struct TileLayer {
    std::vector<int> cells;   // width*height, satýr-major
    int width = 0;
    int height = 0;
    float zOrder = 0.0f;      // çizim sýrasý (küçük: altta)
    glm::vec4 tint{ 1,1,1,1 };  // isteðe baðlý renk

    inline int at(int x, int y) const {
        return cells[y * width + x];
    }
    inline bool empty() const { return width == 0 || height == 0 || cells.empty(); }
};

/// Basit tilemap: 3 katman + atlas parametreleri
class Tilemap {
public:
    // atlasTex: OpenGL texture id, atlasCols: atlas sütun sayýsý (frame indeks hesap için)
    // tileW/H: ekranda tek karonun boyutu (pix)
    Tilemap(GLuint atlasTex, int atlasCols, int tileW, int tileH);

    // CSV yollarýný yükle (virgül veya ';' kabul edilir). true -> baþarý
    bool LoadCSV_BG(const std::string& path);
    bool LoadCSV_World(const std::string& path);
    bool LoadCSV_FG(const std::string& path);

    // Kýsayol: hepsini sýrayla yükle
    bool LoadAll(const std::string& bgCsv,
        const std::string& worldCsv,
        const std::string& fgCsv);

    // Çizim: origin sol-üst köþe (ekran/world koordinatý)
    void Draw(Renderer2D& r, const glm::vec2& origin);

    // Harita boyutu (dünya katmanýna göre döner, yoksa bg/fg bakar)
    glm::vec2 MapPixelSize() const;

    // eriþim
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
        const char* tagForLog); // iç ayrýþtýrýcý

    // Hücre -> atlas frame; -1 boþ
    inline bool isSolid(int v) const { return v >= 0; }

private:
    TileLayer bg, world, fg;
    GLuint atlasTex = 0;
    int atlasCols = 1;
    int tileW = 64, tileH = 64;
};
