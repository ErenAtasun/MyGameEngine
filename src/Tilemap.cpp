#include "Tilemap.h"
#include "Renderer2D.h"   // DrawSpriteAtlas için
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

static inline std::string trim_copy(std::string s) {
    auto notspace = [](int ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
    return s;
}

// UTF-8 BOM varsa düþür
static inline void strip_bom(std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
}

Tilemap::Tilemap(GLuint atlasTex_, int atlasCols_, int tileW_, int tileH_)
    : atlasTex(atlasTex_), atlasCols(atlasCols_), tileW(tileW_), tileH(tileH_) {
    bg.zOrder = 0.0f;    // en altta
    world.zOrder = 0.5f; // ortada
    fg.zOrder = 0.9f;    // en üst
}

bool Tilemap::LoadCSV_BG(const std::string& path) { return loadCSV(path, bg, "bg"); }
bool Tilemap::LoadCSV_World(const std::string& path) { return loadCSV(path, world, "level1"); }
bool Tilemap::LoadCSV_FG(const std::string& path) { return loadCSV(path, fg, "fg"); }

bool Tilemap::LoadAll(const std::string& pbg, const std::string& pworld, const std::string& pfg) {
    bool ok1 = LoadCSV_BG(pbg);
    bool ok2 = LoadCSV_World(pworld);
    bool ok3 = LoadCSV_FG(pfg);
    return ok1 && ok2 && ok3;
}

void Tilemap::SetLayerTint(float r, float g, float b, float a, int layerIndex) {
    glm::vec4 t{ r,g,b,a };
    if (layerIndex == 0) bg.tint = t;
    else if (layerIndex == 1) world.tint = t;
    else if (layerIndex == 2) fg.tint = t;
}
void Tilemap::SetLayerZ(float z, int layerIndex) {
    if (layerIndex == 0) bg.zOrder = z;
    else if (layerIndex == 1) world.zOrder = z;
    else if (layerIndex == 2) fg.zOrder = z;
}

glm::vec2 Tilemap::MapPixelSize() const {
    const TileLayer* base = &world;
    if (base->empty()) base = &bg;
    if (base->empty()) base = &fg;
    if (base->empty()) return { 0,0 };
    return { base->width * (float)tileW, base->height * (float)tileH };
}

bool Tilemap::loadCSV(const std::string& path, TileLayer& out, const char* tag) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Tilemap] HATA: dosya acilamadi: " << path << "\n";
        out = TileLayer{};
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    strip_bom(content);

    std::stringstream ss(content);
    std::string line;

    std::vector<int> tmp;
    int width = -1;
    int row = 0;

    while (std::getline(ss, line, '\n')) {
        row++;
        if (line.size() == 0) continue;

        // satýr ayýrýcýyý otomatik seç: sadece ';' varsa ';', aksi halde ','
        char delim = ',';
        if (line.find(';') != std::string::npos && line.find(',') == std::string::npos)
            delim = ';';

        std::stringstream ls(line);
        std::string cell;
        int colCount = 0;

        while (std::getline(ls, cell, delim)) {
            cell = trim_copy(cell);
            if (cell.empty()) { // boþ ise -1 kabul et
                tmp.push_back(-1);
            }
            else {
                // trailing yorumlarý kes (# veya //)
                size_t hash = cell.find('#');
                size_t sl2 = cell.find("//");
                size_t cut = std::min(hash == std::string::npos ? cell.size() : hash,
                    sl2 == std::string::npos ? cell.size() : sl2);
                if (cut != std::string::npos) cell = trim_copy(cell.substr(0, cut));

                if (cell.empty()) tmp.push_back(-1);
                else {
                    try {
                        // 0x.., 0b.. veya normal sayý destek (std::stoi otomatik 10 taban)
                        int v = std::stoi(cell);
                        tmp.push_back(v);
                    }
                    catch (...) {
                        std::cerr << "[Tilemap] Warning: sayi parse edilemedi: '"
                            << cell << "' (satir " << row << ", sutun " << colCount << ")\n";
                        tmp.push_back(-1);
                    }
                }
            }
            colCount++;
        }

        if (colCount == 0) continue;
        if (width < 0) width = colCount;
        else if (width != colCount) {
            std::cerr << "[Tilemap] Warning: satir " << row
                << " sutun sayisi farkli (" << colCount << " vs " << width << ")\n";
        }
    }

    if (width <= 0) {
        out = TileLayer{};
        std::cerr << "[Tilemap] Uyarý: bos veya gecersiz CSV: " << path << "\n";
        return false;
    }

    // satýr sayýsý
    int height = (int)(tmp.size() / width);
    out.cells = std::move(tmp);
    out.width = width;
    out.height = height;

    std::cout << "[Tilemap] Yüklendi: " << path << " (" << out.width << "x" << out.height << ")\n";
    return true;
}

void Tilemap::Draw(Renderer2D& r, const glm::vec2& origin) {
    if (!atlasTex || atlasCols <= 0) return;

    auto drawLayer = [&](const TileLayer& L) {
        if (L.empty()) return;
        for (int y = 0; y < L.height; ++y) {
            for (int x = 0; x < L.width; ++x) {
                int v = L.at(x, y);
                if (!isSolid(v)) continue;

                glm::vec2 pos = origin + glm::vec2(x * tileW, y * tileH);
                glm::vec2 size = glm::vec2(tileW, tileH);

                // >>>> BURAYI KENDÝ RENDERER’INA UYDURABÝLÝRSÝN <<<<
                r.DrawSpriteAtlas(atlasTex, atlasCols, v, pos, size, L.tint, L.zOrder);
            }
        }
    };

    drawLayer(bg);
    drawLayer(world);
    drawLayer(fg);
}
