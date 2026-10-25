#pragma once
#include <MME/Entity.h>
#include <MME/Physics2D.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mme {

    // Debug çizim ayarları (debug paneli değiştirir, tüm sahneler için geçerli)
    struct SceneDebugDraw {
        bool colliders = false;
        bool velocities = false;
        uint32_t highlightedEntity = 0; // bu ID'li entity'nin collider'ı vurgulanır
    };

    // Entity'lerin sahibi ve sahne sistemlerini (fizik) çalıştıran yapı.
    // Bir Layer genellikle bir Scene tutar ve onu kendi döngüsünden sürer:
    //   OnFixedUpdate -> scene.FixedUpdate(step)
    //   OnUpdate      -> scene.Update(dt)
    //   OnRender      -> BeginScene(camera); scene.Render(); EndScene()
    class Scene {
    public:
        explicit Scene(std::string name = "Scene");
        ~Scene();
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        Entity& CreateEntity(std::string name = "Entity");
        void DestroyEntity(Entity& entity) { entity.Destroy(); } // frame sonunda kaldırılır
        void Clear();                                             // tüm entity'leri hemen kaldırır

        Entity* FindEntity(std::string_view name);
        Entity* FindEntity(uint32_t id);

        void FixedUpdate(float step);   // component'lar, sonra fizik
        void Update(float dt);
        void Render();                  // zLayer'a göre sıralı

        Physics2D& GetPhysics() { return m_Physics; }
        const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_Entities; }
        size_t GetEntityCount() const { return m_Entities.size(); }
        const std::string& GetName() const { return m_Name; }

        // Debug araçları için: şu an var olan tüm sahneler (oluşturulma sırasıyla)
        static const std::vector<Scene*>& GetLiveScenes();
        static SceneDebugDraw& GetDebugDraw();

    private:
        void FlushDestroyed();
        void RenderDebug();

        std::string m_Name;
        std::vector<std::unique_ptr<Entity>> m_Entities;
        std::vector<Entity*> m_RenderOrder; // her frame yeniden kullanılır (allocation yok)
        Physics2D m_Physics;
        uint32_t m_NextID = 1;
    };
}
