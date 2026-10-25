#include <MME/Scene.h>
#include <MME/BoxCollider2D.h>
#include <MME/Renderer2D.h>
#include <MME/RigidBody2D.h>

#include <algorithm>

namespace mme {

    namespace {
        std::vector<Scene*>& LiveScenes() {
            static std::vector<Scene*> s_Scenes;
            return s_Scenes;
        }
    }

    Scene::Scene(std::string name) : m_Name(std::move(name)) {
        LiveScenes().push_back(this);
    }

    Scene::~Scene() {
        Clear();
        auto& scenes = LiveScenes();
        scenes.erase(std::remove(scenes.begin(), scenes.end(), this), scenes.end());
    }

    const std::vector<Scene*>& Scene::GetLiveScenes() { return LiveScenes(); }

    SceneDebugDraw& Scene::GetDebugDraw() {
        static SceneDebugDraw s_Settings;
        return s_Settings;
    }

    Entity& Scene::CreateEntity(std::string name) {
        m_Entities.push_back(std::make_unique<Entity>(*this, m_NextID++, std::move(name)));
        return *m_Entities.back();
    }

    void Scene::Clear() {
        // Sondan başa: geç oluşturulanlar önce yok olur
        while (!m_Entities.empty()) m_Entities.pop_back();
        m_RenderOrder.clear();
    }

    Entity* Scene::FindEntity(std::string_view name) {
        for (auto& e : m_Entities)
            if (!e->IsPendingDestroy() && e->GetName() == name) return e.get();
        return nullptr;
    }

    Entity* Scene::FindEntity(uint32_t id) {
        for (auto& e : m_Entities)
            if (!e->IsPendingDestroy() && e->GetID() == id) return e.get();
        return nullptr;
    }

    // Döngüler indeksle: güncelleme sırasında CreateEntity çağrılabilir.
    void Scene::FixedUpdate(float step) {
        for (size_t i = 0; i < m_Entities.size(); ++i)
            if (m_Entities[i]->IsAlive()) m_Entities[i]->FixedUpdate(step);
        m_Physics.Step(*this, step);
        FlushDestroyed();
    }

    void Scene::Update(float dt) {
        for (size_t i = 0; i < m_Entities.size(); ++i)
            if (m_Entities[i]->IsAlive()) m_Entities[i]->Update(dt);
        FlushDestroyed();
    }

    void Scene::Render() {
        m_RenderOrder.clear();
        for (auto& e : m_Entities)
            if (e->IsAlive()) m_RenderOrder.push_back(e.get());
        // stable: aynı katmandakiler oluşturulma sırasını korur (titreme olmaz)
        std::stable_sort(m_RenderOrder.begin(), m_RenderOrder.end(),
            [](const Entity* a, const Entity* b) { return a->zLayer < b->zLayer; });
        for (Entity* e : m_RenderOrder) e->Render();
        RenderDebug();
    }

    void Scene::RenderDebug() {
        const SceneDebugDraw& dbg = GetDebugDraw();
        if (!dbg.colliders && !dbg.velocities && dbg.highlightedEntity == 0) return;

        for (Entity* e : m_RenderOrder) {
            const auto* col = e->GetComponent<BoxCollider2D>();
            const auto* rb = e->GetComponent<RigidBody2D>();
            const bool highlighted = e->GetID() == dbg.highlightedEntity;

            if (col && (dbg.colliders || highlighted)) {
                glm::vec4 color{ 0.6f, 0.6f, 0.6f, 0.9f };                 // statik
                if (col->isTrigger) color = { 1.0f, 0.85f, 0.2f, 0.9f };
                else if (rb && rb->type == BodyType::Kinematic) color = { 0.3f, 0.8f, 1.0f, 0.9f };
                else if (rb && rb->type == BodyType::Dynamic) color = { 1.0f, 0.5f, 0.2f, 0.9f };
                if (highlighted) color = { 1.0f, 1.0f, 1.0f, 1.0f };
                Renderer2D::DrawRectOutline(col->GetCenter(), col->GetHalfExtents() * 2.0f, color, highlighted ? 3.0f : 1.5f);
            }
            if (rb && dbg.velocities && glm::length(rb->velocity) > 0.0f) {
                // 0.25 sn sonra varacağı yer
                Renderer2D::DrawLine(e->position, e->position + rb->velocity * 0.25f, { 0.4f, 1.0f, 0.4f, 0.9f }, 2.0f);
            }
        }
    }

    void Scene::FlushDestroyed() {
        m_Entities.erase(
            std::remove_if(m_Entities.begin(), m_Entities.end(),
                [](const std::unique_ptr<Entity>& e) { return e->IsPendingDestroy(); }),
            m_Entities.end());
    }
}
