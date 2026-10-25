#pragma once
#include <MME/Entity.h>
#include <MME/OrthoCamera.h>
#include <MME/Physics2D.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mme {

    class CameraComponent;
    class Collider2D;
    class ScriptHost;

    // Debug çizim ayarları (debug paneli değiştirir, tüm sahneler için geçerli)
    struct SceneDebugDraw {
        bool colliders = false;
        bool velocities = false;
        uint32_t highlightedEntity = 0; // bu ID'li entity'nin collider'ı vurgulanır
    };

    // Entity'lerin sahibi ve sahne sistemlerini (fizik, Lua script'leri) çalıştıran yapı.
    // Bir Layer genellikle bir Scene tutar ve onu kendi döngüsünden sürer:
    //   OnFixedUpdate -> scene.FixedUpdate(step)
    //   OnUpdate      -> scene.Update(dt)
    //   OnRender      -> BeginScene(camera); scene.Render(); EndScene()
    // ya da sahnenin kendi kamerasıyla (CameraComponent): scene.RenderRuntime(w, h)
    class Scene {
    public:
        explicit Scene(std::string name = "Scene");
        ~Scene();
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        Entity& CreateEntity(std::string name = "Entity");
        // Belirli ID ile (sahne yükleme / geri alma). ID doluysa yeni bir ID verilir.
        Entity& CreateEntityWithID(uint32_t id, std::string name);
        void DestroyEntity(Entity& entity) { entity.Destroy(); } // frame sonunda kaldırılır
        void Clear();                                             // tüm entity'leri hemen kaldırır
        bool IsClearing() const { return m_Clearing; }

        Entity* FindEntity(std::string_view name);
        Entity* FindEntity(uint32_t id);                          // O(1)
        // İlk aktif ve primary CameraComponent (yoksa ilk aktif kamera, o da yoksa nullptr)
        CameraComponent* GetPrimaryCamera();

        // Entity'nin listedeki (hiyerarşi ve eşit zLayer'da çizim) sırasını değiştirir
        void MoveEntity(Entity& entity, size_t newIndex);

        void FixedUpdate(float step);   // component'lar, sonra fizik
        void Update(float dt);
        void Render();                  // zLayer'a göre sıralı
        void RenderUI();                // Component::OnUIRender (ekran uzayı UI geçişinde)

        // Oyunun kendi görünümü (oynatıcı, editör Play modu): birincil kamerayla
        // dünyayı, ardından UI'ı çizer. Hedef (ekran ya da framebuffer) bağlı olmalı.
        // width/height: hedefin piksel boyutu.
        void RenderRuntime(int width, int height);
        // Sahnenin pencerede gösterildiği dikdörtgen (pencere pikseli, sol-üst):
        // script'lerin fare konumu buna göre çevrilir. Oynatıcıda tüm pencere.
        void SetViewRect(const glm::vec2& offset, const glm::vec2& size) { m_ViewOffset = offset; m_ViewSize = size; }
        glm::vec2 GetViewSize() const { return m_ViewSize; }
        glm::vec2 WindowToView(const glm::vec2& windowPx) const { return windowPx - m_ViewOffset; }
        glm::vec2 WindowToWorld(const glm::vec2& windowPx) const { return m_RuntimeCamera.ScreenToWorld(WindowToView(windowPx)); }
        const OrthoCamera& GetRuntimeCamera() const { return m_RuntimeCamera; }

        // Script'ler bir sonraki sahneye geçmek istediğinde (Scene.Load). Sahibi
        // (editör / oynatıcı) frame sonunda alıp yükler.
        void RequestLoad(std::string path) { m_PendingLoad = std::move(path); }
        bool TakePendingLoad(std::string& path);

        Physics2D& GetPhysics() { return m_Physics; }
        ScriptHost& GetScriptHost();    // ilk script çalıştığında oluşturulur
        bool HasScriptHost() const { return m_Scripts != nullptr; }
        const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_Entities; }
        size_t GetEntityCount() const { return m_Entities.size(); }
        const std::string& GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = std::move(name); }

        // Debug araçları için: şu an var olan tüm sahneler (oluşturulma sırasıyla)
        static const std::vector<Scene*>& GetLiveScenes();
        static SceneDebugDraw& GetDebugDraw();
        // Collider'ın dönmüş dış çizgisi (BeginScene/EndScene arasında)
        static void DrawColliderOutline(const Collider2D& collider, const glm::vec4& color, float thicknessPx = 1.5f);

        // Destroy() edilmiş entity'leri hemen kaldırır (Update bunu frame sonunda yapar;
        // hiç güncellenmeyen sahneler, ör. editörün düzenlenen sahnesi, kendisi çağırır)
        void FlushDestroyed();

    private:
        void RenderDebug();

        std::string m_Name;
        std::vector<std::unique_ptr<Entity>> m_Entities;
        std::unordered_map<uint32_t, Entity*> m_ByID;
        std::vector<Entity*> m_RenderOrder; // her frame yeniden kullanılır (allocation yok)
        Physics2D m_Physics;
        std::unique_ptr<ScriptHost> m_Scripts;
        uint32_t m_NextID = 1;
        bool m_Clearing = false;

        OrthoCamera m_RuntimeCamera{ 1.0f, 1.0f };
        glm::vec2 m_ViewOffset{ 0.0f }, m_ViewSize{ 0.0f };
        std::string m_PendingLoad;
    };
}
