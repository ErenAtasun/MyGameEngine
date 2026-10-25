#pragma once
#include <MME/Event.h>
#include <MME/OrthoCamera.h>

namespace mme {

    // Klavye ile kaydırma + fare tekerleğiyle zoom. Pencere boyutu değişince
    // projeksiyonu kendisi günceller (OnEvent'i katmanınızdan iletin).
    class OrthoCameraController {
    public:
        OrthoCameraController(float viewportWidth, float viewportHeight);

        void OnUpdate(float dt);
        void OnEvent(Event& e);

        OrthoCamera& GetCamera() { return m_Camera; }
        const OrthoCamera& GetCamera() const { return m_Camera; }

        float moveSpeed = 500.0f;   // px/sn (zoom ile ölçeklenir)
        float zoomStep = 0.1f;      // her tekerlek çentiğinde
        float minZoom = 0.1f, maxZoom = 10.0f;
        KeyCode keyUp = Key::KP8, keyDown = Key::KP2, keyLeft = Key::KP4, keyRight = Key::KP6;

    private:
        OrthoCamera m_Camera;
    };
}
