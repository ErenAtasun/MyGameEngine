#include <MME/CameraComponent.h>
#include <MME/Entity.h>
#include <MME/OrthoCamera.h>

namespace mme {

    void CameraComponent::Apply(OrthoCamera& camera, float viewportWidth, float viewportHeight) const {
        const Entity& e = GetEntity();
        camera.SetViewportSize(viewportWidth, viewportHeight);
        // zoom 1 = 1 dünya birimi 1 piksel; viewHeight birim viewportHeight piksele sığmalı
        camera.SetZoom(viewHeight > 0.0f && viewportHeight > 0.0f ? viewHeight / viewportHeight : 1.0f);
        camera.SetPosition(e.position);
        camera.SetRotation(e.rotation);
    }
}
