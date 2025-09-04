#include "UI.h"
#include "Renderer2D.h" // DrawScreenQuad için yeterli
#include "Input.h"      // fare/klavye okuma
#include <cstring>      // (gerekirse) memset, memcpy vb.
// 2) sonra GLFW
// baþka GL header'ý kesinlikle yok

static int s_ScreenW = 0, s_ScreenH = 0;

namespace UI {
    void Begin(int w, int h) { s_ScreenW = w; s_ScreenH = h; }
    void End() {}

    // ekran koordinatýndan NDC/quada çevirip Renderer2D ile çiz
    static void DrawRectPx(float x, float y, float w, float h, unsigned int tex, const float tint[4]) {
        // Renderer2D sahneni ortografik kamera ile çiziyor; UI için full-screen projeksiyon kullanmýyorsan
        // kolay yol: ekran pikselini dünya uzayýna çeviren helper yazmak yerine,
        // Renderer2D'de "DrawScreenQuad(x,y,w,h)" varyantý ekleyebilirsin.
        // Þimdilik: 0,0 sol-üst kabul edelim ve Renderer2D'de böyle bir fonksiyon olduðunu varsayalým:
        Renderer2D::DrawScreenQuad(x, y, w, h, tex, tint);
    }

    void DrawButton(UIButton& b) {
        const auto& m = Input::Mouse();
        // sol-üst referanslý hit-test
        bool inside = (m.x >= b.x && m.x <= b.x + b.w && m.y >= b.y && m.y <= b.y + b.h);
        b.hover = inside;
        bool clicked = false;
        if (inside && m.leftPressed) { b.down = true; }
        if (!Input::Mouse().leftDown && b.down) { // býrakýldý
            clicked = inside; b.down = false;
        }

        float col[4] = { b.tint[0], b.tint[1], b.tint[2], b.tint[3] };
        if (b.down) { col[0] *= 0.8f; col[1] *= 0.8f; col[2] *= 0.8f; }
        else if (b.hover) { col[0] *= 0.9f; col[1] *= 0.9f; col[2] *= 0.9f; }

        DrawRectPx(b.x, b.y, b.w, b.h, b.tex, col);

        if (clicked && b.onClick) b.onClick();
    }
}
