#pragma once
#include <string>
#include <functional>

struct UIButton {
    // ekran koordinat� (px)
    float x, y;   // sol-�st
    float w, h;
    unsigned int tex = 0; // 0 ise beyaz quad
    float tint[4] = { 1,1,1,1 };
    std::string label; // �imdilik kullanmayaca��z (metni ad�m 3'te)
    bool hover = false, down = false;

    // olaylar
    std::function<void()> onClick;
};

namespace UI {
    void Begin(int screenW, int screenH);
    void DrawButton(UIButton& b); // hem �izer hem input i�ler
    void End();
    void DrawTextSimple(const char* text, float x, float y, float size, const float color[4]);
}
