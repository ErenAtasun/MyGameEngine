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
        if (inside && m.leftPressed) { b.down = true; clicked = true; }
        if (!Input::Mouse().leftDown && b.down) { // b�rak�ld�
            clicked = inside; b.down = false;
        }

        float col[4] = { b.tint[0], b.tint[1], b.tint[2], b.tint[3] };
        if (b.down) { col[0] *= 0.8f; col[1] *= 0.8f; col[2] *= 0.8f; }
        else if (b.hover) { col[0] *= 0.9f; col[1] *= 0.9f; col[2] *= 0.9f; }

        DrawRectPx(b.x, b.y, b.w, b.h, b.tex, col);

        if (clicked && b.onClick) b.onClick();
    }

    // 5x7 bitmap glyphs for characters used: 'A','D','E','G','I','L','N','O','R','S','W',' ' (space)
    static bool glyphPixel(char c, int gx, int gy) {
        // gx:0..4, gy:0..6 (top to bottom)
        static const unsigned char A[7] = {0x0,0x1E,0x21,0x3F,0x21,0x21,0x0};
        static const unsigned char D[7] = {0x0,0x3E,0x21,0x21,0x21,0x3E,0x0};
        static const unsigned char E[7] = {0x0,0x3F,0x20,0x3E,0x20,0x3F,0x0};
        static const unsigned char G[7] = {0x0,0x1E,0x20,0x27,0x21,0x1E,0x0};
        static const unsigned char I[7] = {0x0,0x1F,0x04,0x04,0x04,0x1F,0x0};
        static const unsigned char L[7] = {0x0,0x20,0x20,0x20,0x20,0x3F,0x0};
        static const unsigned char N[7] = {0x0,0x21,0x31,0x29,0x25,0x23,0x0};
        static const unsigned char O[7] = {0x0,0x1E,0x21,0x21,0x21,0x1E,0x0};
        static const unsigned char R[7] = {0x0,0x3E,0x21,0x3E,0x24,0x23,0x0};
        static const unsigned char S[7] = {0x0,0x1F,0x20,0x1E,0x01,0x3E,0x0};
        static const unsigned char W[7] = {0x0,0x21,0x21,0x2A,0x2A,0x14,0x0};
        const unsigned char* rows = nullptr;
        switch (c) {
            case 'A': rows = A; break;
            case 'D': rows = D; break;
            case 'E': rows = E; break;
            case 'G': rows = G; break;
            case 'I': rows = I; break;
            case 'L': rows = L; break;
            case 'N': rows = N; break;
            case 'O': rows = O; break;
            case 'R': rows = R; break;
            case 'S': rows = S; break;
            case 'W': rows = W; break;
            case ' ': return false;
            default:  return false;
        }
        unsigned char rowBits = rows[gy & 7];
        int bit = 4 - gx; // leftmost is bit 4
        return (rowBits >> bit) & 1;
    }

    void DrawTextSimple(const char* text, float x, float y, float size, const float color[4]) {
        if (!text) return;
        const float pixel = size / 7.0f; // 7 rows
        const float glyphW = pixel * 5.0f;
        const float glyphH = pixel * 7.0f;
        const float advance = glyphW + pixel; // 1 px spacing
        unsigned int tex = 0; // white
        for (const char* p = text; *p; ++p) {
            char c = (char)toupper(*p);
            for (int gy = 0; gy < 7; ++gy) {
                for (int gx = 0; gx < 5; ++gx) {
                    if (glyphPixel(c, gx, gy)) {
                        float rx = x + (float)(p - text) * advance + gx * pixel;
                        float ry = y + gy * pixel;
                        Renderer2D::DrawScreenQuad(rx, ry, pixel, pixel, tex, color);
                    }
                }
            }
        }
    }
}
