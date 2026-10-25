#pragma once
#include <MME/Assets.h>

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mme {

    // UTF-8 çözümü: text[i]'den bir kod noktası okur, i'yi ilerletir.
    // Geçersiz bayt dizilerinde U+FFFD döner.
    uint32_t DecodeUtf8(std::string_view text, size_t& i);

    // Glif atlası + metrikler. Hem yerleşik 5x7 bitmap font hem TTF dosyaları
    // bu sınıfla temsil edilir; UI::Text ikisini aynı yoldan çizer.
    // Metrikler "font birimi" cinsindendir: ölçek = istenen boyut / GetPixelHeight().
    class Font {
    public:
        struct Glyph {
            glm::vec2 uvMin{ 0.0f }, uvMax{ 0.0f };  // UI çizimi için (üst = uvMin.y)
            glm::vec2 offset{ 0.0f };   // taban çizgisindeki kalemden glifin sol-üstüne (+y aşağı)
            glm::vec2 size{ 0.0f };     // glif dörtgeni
            float advance = 0.0f;       // sonraki karaktere ilerleme
            bool visible = false;       // boşluk gibi mürekkepsiz glifler için false
        };

        // TTF'yi verilen piksel yüksekliğinde atlasa basar: ASCII, Latin-1 ve
        // Latin Extended-A (Türkçe dahil). OpenGL context yoksa yalnızca metrikler yüklenir.
        static std::shared_ptr<Font> LoadTTF(const std::string& relativePath, float bakePixelHeight = 48.0f);
        static std::shared_ptr<Font> CreateBuiltin(); // motorun 5x7 bitmap fontu

        ~Font();
        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        // Yoksa en yakın karşılık (ş -> s), o da yoksa '?' döner
        const Glyph& GetGlyph(uint32_t codepoint) const;
        bool HasGlyph(uint32_t codepoint) const { return m_Glyphs.count(codepoint) != 0; }

        float GetAscent() const { return m_Ascent; }       // taban çizgisinin üstü (pozitif)
        float GetDescent() const { return m_Descent; }     // taban çizgisinin altı (negatif)
        float GetLineHeight() const { return m_LineHeight; }
        float GetPixelHeight() const { return m_Ascent - m_Descent; }
        TextureID GetAtlas() const { return m_Atlas; }
        const std::string& GetName() const { return m_Name; }

    private:
        Font() = default;

        std::string m_Name;
        std::unordered_map<uint32_t, Glyph> m_Glyphs;
        Glyph m_Missing;
        float m_Ascent = 0.0f, m_Descent = 0.0f, m_LineHeight = 0.0f;
        TextureID m_Atlas = 0;
    };
}
