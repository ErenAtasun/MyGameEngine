#pragma once
#include <MME/Component.h>

#include <cstdint>
#include <memory>
#include <string>

namespace mme {

    // Entity'ye Lua davranışı ekler. Script, sahne ilk kez güncellendiğinde
    // yüklenir (editörde sahne düzenlenirken çalışmaz). Script şu fonksiyonları
    // tanımlayabilir; hepsi isteğe bağlıdır ve ilk parametre self'tir:
    //
    //   properties = { speed = 300, jumpHeight = 120 }  -- inspector'da düzenlenir
    //
    //   function OnStart(self) end                 -- ilk güncellemeden önce bir kez
    //   function OnUpdate(self, dt) end            -- her frame
    //   function OnFixedUpdate(self, step) end     -- sabit adım (fizikten önce)
    //   function OnCollision(self, other, normal) end
    //   function OnTrigger(self, other) end
    //   function OnUI(self) end                    -- ekran uzayı (UI.Text ...)
    //   function OnDestroy(self) end
    //
    // self.entity bu entity'dir; properties değerleri self'e kopyalanır (self.speed).
    // Script dosyası değişince yeniden yüklenir; self'teki durum korunur.
    class ScriptComponent : public Component {
    public:
        ScriptComponent();
        ~ScriptComponent() override;

        std::string script;       // assets'e göre yol, ör. "scripts/player.lua"
        std::string properties;   // inspector'dan değiştirilen properties değerleri (JSON nesnesi)

        void OnFixedUpdate(float step) override;
        void OnUpdate(float dt) override;
        void OnUIRender() override;
        void OnCollisionEnter(const Collision& collision) override;
        void OnTriggerEnter(Entity& other) override;
        void OnDestroy() override;

        bool IsRunning() const;             // yüklendi ve hata yok
        const std::string& GetError() const { return m_Error; }

        struct Instance;                    // Lua tarafı (motor içi)
        Instance* GetInstance() const { return m_Instance.get(); }

    private:
        bool EnsureStarted();
        void CheckReload();
        void Fail(const std::string& message);

        std::unique_ptr<Instance> m_Instance;
        std::string m_LoadedScript;
        uint64_t m_LoadedVersion = 0;
        bool m_Started = false;
        bool m_Failed = false;              // hata: dosya değişene kadar çağrılmaz
        std::string m_Error;
    };
}
