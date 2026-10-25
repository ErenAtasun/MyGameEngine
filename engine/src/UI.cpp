#include <MME/UI.h>
#include <MME/Font.h>
#include <MME/Input.h>
#include <MME/Renderer2D.h>

#include <algorithm>
#include <cmath>

namespace mme::UI {

    namespace {
        std::shared_ptr<Font> s_Builtin;   // motorun 5x7 fontu
        std::shared_ptr<Font> s_Default;   // SetFont ile seçilen (yoksa builtin)

        const Font& Resolve(const Font* font) {
            if (font) return *font;
            if (s_Default) return *s_Default;
            if (!s_Builtin) s_Builtin = Font::CreateBuiltin(); // GL olmadan da ölçüm yapılabilsin
            return *s_Builtin;
        }
    }

    void Init() {
        s_Builtin = Font::CreateBuiltin();
    }

    void Shutdown() {
        s_Default.reset();
        s_Builtin.reset();
    }

    void SetFont(std::shared_ptr<Font> font) { s_Default = std::move(font); }
    std::shared_ptr<Font> GetFont() { return s_Default ? s_Default : s_Builtin; }

    void Text(std::string_view text, const glm::vec2& topLeft, float size, const glm::vec4& color, const Font* fontPtr) {
        const Font& font = Resolve(fontPtr);
        const float scale = size / font.GetPixelHeight();

        SpriteUVDesc g;
        g.texture = font.GetAtlas();
        g.tint = color;
        g.origin = { 0.0f, 0.0f };

        float penX = topLeft.x;
        // Taban çizgisini piksele hizala: yatay çizgiler bulanıklaşmasın
        float baseline = std::round(topLeft.y + font.GetAscent() * scale);
        for (size_t i = 0; i < text.size();) {
            const uint32_t cp = DecodeUtf8(text, i);
            if (cp == '\n') {
                penX = topLeft.x;
                baseline += std::round(font.GetLineHeight() * scale);
                continue;
            }
            const Font::Glyph& glyph = font.GetGlyph(cp);
            if (glyph.visible) {
                g.pos = { penX + glyph.offset.x * scale, baseline + glyph.offset.y * scale };
                g.size = glyph.size * scale;
                g.uvMin = glyph.uvMin;
                g.uvMax = glyph.uvMax;
                Renderer2D::DrawSpriteUV(g);
            }
            penX += glyph.advance * scale;
        }
    }

    glm::vec2 MeasureText(std::string_view text, float size, const Font* fontPtr) {
        if (text.empty()) return { 0.0f, 0.0f };
        const Font& font = Resolve(fontPtr);
        const float scale = size / font.GetPixelHeight();

        float width = 0.0f, lineWidth = 0.0f, penX = 0.0f;
        int lines = 1;
        for (size_t i = 0; i < text.size();) {
            const uint32_t cp = DecodeUtf8(text, i);
            if (cp == '\n') {
                width = std::max(width, lineWidth);
                lineWidth = penX = 0.0f;
                ++lines;
                continue;
            }
            const Font::Glyph& glyph = font.GetGlyph(cp);
            // Genişlik son glifin mürekkebinde biter (sağındaki boşluk sayılmaz);
            // mürekkepsiz glifler (boşluk) ilerlemesi kadar yer kaplar.
            const float right = glyph.visible ? penX + (glyph.offset.x + glyph.size.x) * scale
                                              : penX + glyph.advance * scale;
            lineWidth = std::max(lineWidth, right);
            penX += glyph.advance * scale;
        }
        width = std::max(width, lineWidth);
        const float height = font.GetPixelHeight() * scale + (float)(lines - 1) * font.GetLineHeight() * scale;
        return { width, height };
    }

    void TextCentered(std::string_view text, const glm::vec2& center, float size, const glm::vec4& color, const Font* font) {
        Text(text, center - MeasureText(text, size, font) * 0.5f, size, color, font);
    }

    bool Button(UIButton& b) {
        const glm::vec2 m = Input::MousePosition();
        const bool inside = m.x >= b.pos.x && m.x <= b.pos.x + b.size.x &&
                            m.y >= b.pos.y && m.y <= b.pos.y + b.size.y;
        b.hover = inside;

        if (inside && Input::MousePressed(Mouse::Left)) b.down = true;
        bool clicked = false;
        if (b.down && !Input::MouseDown(Mouse::Left)) {
            clicked = inside;
            b.down = false;
        }

        glm::vec4 col = b.color;
        const float shade = b.down ? 0.8f : (b.hover ? 0.9f : 1.0f);
        col.r *= shade; col.g *= shade; col.b *= shade;
        Renderer2D::DrawScreenRect(b.pos, b.size, col, b.texture);

        if (!b.label.empty())
            TextCentered(b.label, b.pos + b.size * 0.5f, b.size.y * 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

        if (clicked && b.onClick) b.onClick();
        return clicked;
    }
}
