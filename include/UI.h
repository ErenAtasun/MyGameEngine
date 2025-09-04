#pragma once
#include <string>
#include <functional>

struct UIButton {
    // ekran koordinatý (px)
    float x, y;   // sol-üst
    float w, h;
    unsigned int tex = 0; // 0 ise beyaz quad
    float tint[4] = { 1,1,1,1 };
    std::string label; // þimdilik kullanmayacaðýz (metni adým 3'te)
    bool hover = false, down = false;

    // olaylar
    std::function<void()> onClick;
};

namespace UI {
    void Begin(int screenW, int screenH);
    void DrawButton(UIButton& b); // hem çizer hem input iþler
    void End();
}
