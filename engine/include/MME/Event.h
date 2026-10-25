#pragma once
#include <MME/KeyCodes.h>

namespace mme {

    enum class EventType {
        WindowClose,
        WindowResize,
        KeyPressed,
        KeyReleased,
        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseScrolled,
    };

    // Tek, düz bir olay yapısı: yalnızca türe ait alanlar anlamlıdır.
    // Katmanlar üstten alta dolaşılır; handled = true olunca yayılım durur.
    struct Event {
        EventType type;
        bool handled = false;

        int width = 0, height = 0;   // WindowResize: framebuffer piksel
        KeyCode key = 0;             // Key*
        bool repeat = false;         // KeyPressed: basılı tutma tekrarı
        MouseButton button = 0;      // MouseButton*
        float x = 0.0f, y = 0.0f;    // MouseMoved: konum (fb piksel, sol-üst); MouseScrolled: kaydırma miktarı
    };
}
