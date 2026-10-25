#include <MME/Font.h>
#include <MME/Log.h>

#include <GLFW/glfw3.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <fstream>
#include <iterator>
#include <vector>

namespace mme {

    // ------------------------------------------------------------------ UTF-8
    uint32_t DecodeUtf8(std::string_view text, size_t& i) {
        const auto byte = [&](size_t k) { return (uint32_t)(unsigned char)text[k]; };
        const uint32_t c = byte(i++);
        if (c < 0x80) return c;

        int extra;
        uint32_t cp;
        if ((c & 0xE0) == 0xC0)      { extra = 1; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07; }
        else return 0xFFFD;            // tek başına devam baytı ya da geçersiz başlangıç

        for (int k = 0; k < extra; ++k) {
            if (i >= text.size() || (byte(i) & 0xC0) != 0x80) return 0xFFFD; // eksik dizi: baytı tüketme
            cp = (cp << 6) | (byte(i++) & 0x3F);
        }
        return cp;
    }

    namespace {
        // ------------------------------------------------------------ yerleşik 5x7 font
        // Klasik 5x7 font, ASCII 32..126. Her karakter 5 sütun; her baytta
        // bit 0 en üst satır, bit 6 en alt satır.
        const unsigned char kFont5x7[95][5] = {
            {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, {0x00,0x07,0x00,0x07,0x00}, // ' ' ! "
            {0x14,0x7F,0x14,0x7F,0x14}, {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, // # $ %
            {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, {0x00,0x1C,0x22,0x41,0x00}, // & ' (
            {0x00,0x41,0x22,0x1C,0x00}, {0x08,0x2A,0x1C,0x2A,0x08}, {0x08,0x08,0x3E,0x08,0x08}, // ) * +
            {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, {0x00,0x60,0x60,0x00,0x00}, // , - .
            {0x20,0x10,0x08,0x04,0x02}, {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, // / 0 1
            {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10}, // 2 3 4
            {0x27,0x45,0x45,0x45,0x39}, {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, // 5 6 7
            {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, {0x00,0x36,0x36,0x00,0x00}, // 8 9 :
            {0x00,0x56,0x36,0x00,0x00}, {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, // ; < =
            {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, {0x32,0x49,0x79,0x41,0x3E}, // > ? @
            {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, // A B C
            {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01}, // D E F
            {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, // G H I
            {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40}, // J K L
            {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, // M N O
            {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46}, // P Q R
            {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, // S T U
            {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63}, // V W X
            {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00}, // Y Z [
            {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, {0x04,0x02,0x01,0x02,0x04}, // \ ] ^
            {0x40,0x40,0x40,0x40,0x40}, {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78}, // _ ` a
            {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20}, {0x38,0x44,0x44,0x48,0x7F}, // b c d
            {0x38,0x54,0x54,0x54,0x18}, {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E}, // e f g
            {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00}, {0x20,0x40,0x44,0x3D,0x00}, // h i j
            {0x7F,0x10,0x28,0x44,0x00}, {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x18,0x04,0x78}, // k l m
            {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38}, {0x7C,0x14,0x14,0x14,0x08}, // n o p
            {0x08,0x14,0x14,0x18,0x7C}, {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20}, // q r s
            {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C}, {0x1C,0x20,0x40,0x20,0x1C}, // t u v
            {0x3C,0x40,0x30,0x40,0x3C}, {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C}, // w x y
            {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00}, {0x00,0x00,0x7F,0x00,0x00}, // z { |
            {0x00,0x41,0x36,0x08,0x00}, {0x08,0x04,0x08,0x10,0x08},                               // } ~
        };

        // Fontta olmayan harfler için en yakın ASCII karşılık (Türkçe + yaygın aksanlılar)
        uint32_t AsciiFallback(uint32_t cp) {
            switch (cp) {
                case 0x00E7: return 'c'; case 0x00C7: return 'C';   // ç Ç
                case 0x011F: return 'g'; case 0x011E: return 'G';   // ğ Ğ
                case 0x0131: return 'i'; case 0x0130: return 'I';   // ı İ
                case 0x00F6: return 'o'; case 0x00D6: return 'O';   // ö Ö
                case 0x015F: return 's'; case 0x015E: return 'S';   // ş Ş
                case 0x00FC: return 'u'; case 0x00DC: return 'U';   // ü Ü
                case 0x00E2: case 0x00E0: case 0x00E1: case 0x00E4: return 'a';
                case 0x00C2: case 0x00C0: case 0x00C1: case 0x00C4: return 'A';
                case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: return 'e';
                case 0x00C9: case 0x00C8: case 0x00CA: case 0x00CB: return 'E';
                case 0x00EE: case 0x00ED: case 0x00EC: case 0x00EF: return 'i';
                case 0x00F4: case 0x00F3: case 0x00F2: return 'o';
                case 0x00FB: case 0x00FA: case 0x00F9: return 'u';
                case 0x00F1: return 'n'; case 0x00D1: return 'N';
                default: return 0;
            }
        }

        bool HasGLContext() { return glfwGetCurrentContext() != nullptr; }
    }

    Font::~Font() {
        Assets::DestroyTexture(m_Atlas);
    }

    const Font::Glyph& Font::GetGlyph(uint32_t cp) const {
        if (auto it = m_Glyphs.find(cp); it != m_Glyphs.end()) return it->second;
        if (const uint32_t ascii = AsciiFallback(cp))
            if (auto it = m_Glyphs.find(ascii); it != m_Glyphs.end()) return it->second;
        if (auto it = m_Glyphs.find('?'); it != m_Glyphs.end()) return it->second;
        return m_Missing;
    }

    std::shared_ptr<Font> Font::CreateBuiltin() {
        constexpr int kGlyphW = 5, kGlyphH = 7;           // glif pikseli
        constexpr int kCellW = 6, kCellH = 8;             // atlas hücresi (1 px boşluk)
        constexpr int kCols = 16, kRows = 6;              // 96 karakter yeri
        constexpr int kW = kCols * kCellW, kH = kRows * kCellH;

        std::shared_ptr<Font> font(new Font());
        font->m_Name = "builtin-5x7";
        font->m_Ascent = (float)kGlyphH;
        font->m_Descent = 0.0f;
        font->m_LineHeight = (float)kCellH;
        font->m_Missing.advance = (float)kCellW;

        std::vector<uint32_t> pixels((size_t)kW * kH, 0u);
        for (int i = 0; i < 95; ++i) {
            const int cx = (i % kCols) * kCellW;
            const int cy = (i / kCols) * kCellH;
            bool ink = false;
            for (int gx = 0; gx < kGlyphW; ++gx)
                for (int gy = 0; gy < kGlyphH; ++gy)
                    if (kFont5x7[i][gx] & (1 << gy)) {
                        pixels[(size_t)(cy + gy) * kW + (size_t)(cx + gx)] = 0xFFFFFFFFu;
                        ink = true;
                    }

            Glyph g;
            // Dokularda v=0 altta; UI'da +y aşağı -> quad'ın üstü glifin üstünü alır
            g.uvMin = { (float)cx / kW,             1.0f - (float)cy / kH };
            g.uvMax = { (float)(cx + kGlyphW) / kW, 1.0f - (float)(cy + kGlyphH) / kH };
            g.offset = { 0.0f, -(float)kGlyphH };
            g.size = { (float)kGlyphW, (float)kGlyphH };
            g.advance = (float)kCellW;
            g.visible = ink;
            font->m_Glyphs[(uint32_t)(32 + i)] = g;
        }

        if (HasGLContext())
            font->m_Atlas = Assets::CreateTexture(kW, kH, pixels.data(), TextureFilter::Nearest);
        return font;
    }

    std::shared_ptr<Font> Font::LoadTTF(const std::string& relativePath, float bakePixelHeight) {
        const std::string path = Assets::GetPath(relativePath);
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            MME_ERROR("[Font] Dosya acilamadi: ", path);
            return nullptr;
        }
        const std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        stbtt_fontinfo info;
        if (data.empty() || !stbtt_InitFont(&info, data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0))) {
            MME_ERROR("[Font] Gecerli bir TrueType dosyasi degil: ", path);
            return nullptr;
        }

        std::shared_ptr<Font> font(new Font());
        font->m_Name = relativePath;

        const float scale = stbtt_ScaleForPixelHeight(&info, bakePixelHeight);
        int ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        font->m_Ascent = ascent * scale;
        font->m_Descent = descent * scale;
        font->m_LineHeight = (ascent - descent + lineGap) * scale;
        font->m_Missing.advance = bakePixelHeight * 0.5f;

        // ASCII, Latin-1 Supplement, Latin Extended-A (ç ğ ı ö ş ü İ Ğ Ş ...)
        struct Range { int first, count; };
        const Range ranges[] = { { 32, 95 }, { 160, 96 }, { 256, 128 } };
        constexpr int kRangeCount = (int)(sizeof(ranges) / sizeof(ranges[0]));

        std::vector<stbtt_packedchar> chars[kRangeCount];
        stbtt_pack_range packRanges[kRangeCount];
        for (int r = 0; r < kRangeCount; ++r) {
            chars[r].resize((size_t)ranges[r].count);
            packRanges[r] = {};
            packRanges[r].font_size = bakePixelHeight;
            packRanges[r].first_unicode_codepoint_in_range = ranges[r].first;
            packRanges[r].num_chars = ranges[r].count;
            packRanges[r].chardata_for_range = chars[r].data();
        }

        // Sığana kadar atlası büyüt
        int size = 512;
        std::vector<unsigned char> alpha;
        for (;; size *= 2) {
            alpha.assign((size_t)size * size, 0);
            stbtt_pack_context pc;
            if (!stbtt_PackBegin(&pc, alpha.data(), size, size, 0, 2, nullptr)) return nullptr;
            const int ok = stbtt_PackFontRanges(&pc, data.data(), 0, packRanges, kRangeCount);
            stbtt_PackEnd(&pc);
            if (ok) break;
            if (size >= 4096) {
                MME_ERROR("[Font] Atlas 4096'ya sigmadi: ", relativePath, " (bakePixelHeight kucultulmeli)");
                return nullptr;
            }
        }

        for (int r = 0; r < kRangeCount; ++r) {
            for (int k = 0; k < ranges[r].count; ++k) {
                const int cp = ranges[r].first + k;
                if (stbtt_FindGlyphIndex(&info, cp) == 0) continue; // fontta yok: fallback kullanılır

                float x = 0.0f, y = 0.0f;
                stbtt_aligned_quad q;
                stbtt_GetPackedQuad(chars[r].data(), size, size, k, &x, &y, &q, 0);

                Glyph g;
                g.offset = { q.x0, q.y0 };
                g.size = { q.x1 - q.x0, q.y1 - q.y0 };
                g.advance = x;
                // stb'de t yukarıdan aşağı; bizim dokularda v=0 altta
                g.uvMin = { q.s0, 1.0f - q.t0 };
                g.uvMax = { q.s1, 1.0f - q.t1 };
                g.visible = g.size.x > 0.0f && g.size.y > 0.0f;
                font->m_Glyphs[(uint32_t)cp] = g;
            }
        }

        if (HasGLContext()) {
            std::vector<uint32_t> rgba(alpha.size());
            for (size_t i = 0; i < alpha.size(); ++i)
                rgba[i] = 0x00FFFFFFu | ((uint32_t)alpha[i] << 24);   // beyaz, alfa = glif
            font->m_Atlas = Assets::CreateTexture(size, size, rgba.data(), TextureFilter::Linear, true);
        }

        MME_INFO("[Font] Yuklendi: ", relativePath, " (", font->m_Glyphs.size(), " glif, atlas ", size, "x", size, ")");
        return font;
    }
}
