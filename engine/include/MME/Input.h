#pragma once
#include <MME/KeyCodes.h>
#include <glm/glm.hpp>

// Klavye/fare durumu. Application her frame'in başında "bu frame" bayraklarını
// temizler, sonra olayları işler; oyun kodu Update içinde güvenle okuyabilir.
//
// Not: Pressed/Released tek frame'liktir; bunları OnUpdate içinde okuyun.
// OnFixedUpdate bir frame'de 0 ya da birden çok kez çalışabilir; orada Down kullanın.
namespace mme::Input {

    bool KeyDown(KeyCode key);       // basılı tutuluyor
    bool KeyPressed(KeyCode key);    // bu frame basıldı (tekrarlar hariç)
    bool KeyReleased(KeyCode key);   // bu frame bırakıldı

    bool MouseDown(MouseButton button);
    bool MousePressed(MouseButton button);
    bool MouseReleased(MouseButton button);

    glm::vec2 MousePosition();       // framebuffer pikseli, (0,0) sol-üst
    float ScrollDelta();             // bu frame'deki dikey tekerlek hareketi
}
