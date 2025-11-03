#include "UI.h"
#include "Renderer2D.h" // DrawScreenQuad i�in yeterli
#include "Input.h"      // fare/klavye okuma
#include <cstring>      // (gerekirse) memset, memcpy vb.
// 2) sonra GLFW
// ba�ka GL header'� kesinlikle yok

static int s_ScreenW = 0, s_ScreenH = 0;

namespace UI {
    void Begin(int w, int h) { s_ScreenW = w; s_ScreenH = h; }
    void End() {}

    // ekran koordinat�ndan NDC/quada �evirip Renderer2D ile �iz
    static void DrawRectPx(float x, float y, float w, float h, unsigned int tex, const float tint[4]) {
        SpriteDesc s{};
        s.texture = tex;
        s.pos = { x + w * 0.5f, y + h * 0.5f }; // UI projeksiyonu (0,0 sol-ust, +y asagi)
        s.size = { w, h };
        s.tint = { tint[0], tint[1], tint[2], tint[3] };
        Renderer2D::DrawSprite(s);
    }

    void DrawButton(UIButton& b) {
        const auto& m = Input::Mouse();
        // sol-�st referansl� hit-test
        bool inside = (m.x >= b.x && m.x <= b.x + b.w && m.y >= b.y && m.y <= b.y + b.h);
        b.hover = inside;
        bool clicked = false;
        if (inside && m.leftPressed) { b.down = true; }
        if (!Input::Mouse().leftDown && b.down) { // b�rak�ld�
            clicked = inside; b.down = false;
        }

        float col[4] = { b.tint[0], b.tint[1], b.tint[2], b.tint[3] };
        if (b.down) { col[0] *= 0.8f; col[1] *= 0.8f; col[2] *= 0.8f; }
        else if (b.hover) { col[0] *= 0.9f; col[1] *= 0.9f; col[2] *= 0.9f; }

        DrawRectPx(b.x, b.y, b.w, b.h, b.tex, col);

        if (clicked && b.onClick) b.onClick();
    }
}
