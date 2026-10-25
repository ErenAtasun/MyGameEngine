#pragma once
#include <MME/Component.h>
#include <glm/glm.hpp>

namespace mme {

    class OrthoCamera;

    // Sahnenin kamerası: entity'nin konumu ve rotasyonu kameranınkidir.
    // Editörde kurulan oyunlar (ve oynatıcı) sahneyi birincil kameradan çizer.
    class CameraComponent : public Component {
    public:
        float viewHeight = 720.0f;   // dikeyde görünen dünya birimi (pencere boyundan bağımsız)
        bool primary = true;         // birden fazla kamera varsa ilk primary olan kullanılır
        glm::vec4 background{ 0.10f, 0.11f, 0.15f, 1.0f };

        // Kamerayı bu bileşene göre ayarlar (viewport: hedefin piksel boyutu)
        void Apply(OrthoCamera& camera, float viewportWidth, float viewportHeight) const;
    };
}
