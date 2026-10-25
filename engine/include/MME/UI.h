#pragma once
#include <MME/Assets.h>
#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace mme {

    class Font;

    struct UIButton {
        glm::vec2 pos{ 0.0f };            // sol-üst, ekran pikseli
        glm::vec2 size{ 120.0f, 40.0f };
        glm::vec4 color{ 0.25f, 0.25f, 0.30f, 1.0f };
        std::string label;                // boş değilse ortasına yazılır (UTF-8)
        TextureID texture = 0;            // 0 = düz renk

        std::function<void()> onClick;

        // Durum (UI::Button günceller)
        bool hover = false, down = false;
    };
}

// Ekran uzayı UI: yalnızca Layer::OnUIRender içinde kullanılır
// ((0,0) sol-üst, +y aşağı, framebuffer pikseli).
namespace mme::UI {

    void Init();      // Application çağırır
    void Shutdown();

    // Varsayılan font: nullptr verilirse motorun 5x7 bitmap fontu
    void SetFont(std::shared_ptr<Font> font);
    std::shared_ptr<Font> GetFont();

    // Metin UTF-8'dir; '\n' yeni satır. size = satırın piksel yüksekliği
    // (yerleşik fontta büyük harf yüksekliği). font = nullptr -> varsayılan font.
    void Text(std::string_view text, const glm::vec2& topLeft, float size, const glm::vec4& color, const Font* font = nullptr);
    void TextCentered(std::string_view text, const glm::vec2& center, float size, const glm::vec4& color, const Font* font = nullptr);
    glm::vec2 MeasureText(std::string_view text, float size, const Font* font = nullptr);

    // Çizer ve girdiyi işler. Tıklandıysa true döner (ve onClick çağrılır).
    // Tıklama = butonun üstünde basıp yine üstünde bırakmak.
    bool Button(UIButton& button);
}
