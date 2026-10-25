#pragma once
#include <functional>

namespace mme {
    class Component;
    class Entity;
    struct ComponentInfo;
    struct FieldInfo;
}

// ComponentRegistry tabanlı ImGui inspector'ı: F1 debug paneli ve editör aynı
// kodu kullanır. ImGui frame'i içinde (OnImGuiRender) çağrılmalıdır.
namespace mme::InspectorUI {

    // Asset tarayıcıdan sürüklenen dosya: payload, '\0' ile biten UTF-8 göreli yol.
    // Doku alanları resim dosyalarını, metin alanları her dosyayı kabul eder.
    inline constexpr const char* kAssetPayload = "MME_ASSET";

    struct Hooks {
        // Bir değer değişmeden hemen önce çağrılır (sürükleme başında bir kez):
        // editör burada geri alma için sahnenin anlık görüntüsünü alır.
        std::function<void()> beforeEdit;
    };

    // İsim, aktiflik, dönüşüm, tüm component'ler ve "Component ekle" menüsü.
    // Bir şey değiştiyse true döner.
    bool DrawEntity(Entity& entity, const Hooks& hooks = {});
    bool DrawComponent(const ComponentInfo& info, Component& component, const Hooks& hooks = {});
    bool DrawField(const FieldInfo& field, Component& component, const Hooks& hooks = {});
}
