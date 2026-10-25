#include <MME/Scene.h>
#include <MME/CameraComponent.h>
#include <MME/Collider2D.h>
#include <MME/Renderer2D.h>
#include <MME/RigidBody2D.h>
#include "ScriptHost.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

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
        // Silinmeyi bekleyen bir entity bu ID'yi hâlâ tutuyor olabilir (geri alma)
        while (m_ByID.count(m_NextID)) ++m_NextID;
        m_Entities.push_back(std::make_unique<Entity>(*this, m_NextID++, std::move(name)));
        Entity& e = *m_Entities.back();
        m_ByID[e.GetID()] = &e;
        return e;
    }

    Entity& Scene::CreateEntityWithID(uint32_t id, std::string name) {
        if (id == 0 || m_ByID.count(id)) return CreateEntity(std::move(name)); // dolu ID: yenisi verilir
        m_Entities.push_back(std::make_unique<Entity>(*this, id, std::move(name)));
        m_NextID = std::max(m_NextID, id + 1);
        Entity& e = *m_Entities.back();
        m_ByID[id] = &e;
        return e;
    }

    void Scene::Clear() {
        m_Clearing = true;
        m_Physics.Clear();
        // Sondan başa: geç oluşturulanlar önce yok olur
        while (!m_Entities.empty()) m_Entities.pop_back();
        m_ByID.clear();
        m_RenderOrder.clear();
        m_Scripts.reset();   // yeni içerik temiz bir Lua durumuyla başlar
        m_Clearing = false;
    }

    Entity* Scene::FindEntity(std::string_view name) {
        for (auto& e : m_Entities)
            if (!e->IsPendingDestroy() && e->GetName() == name) return e.get();
        return nullptr;
    }

    Entity* Scene::FindEntity(uint32_t id) {
        const auto it = m_ByID.find(id);
        return it != m_ByID.end() && !it->second->IsPendingDestroy() ? it->second : nullptr;
    }

    void Scene::MoveEntity(Entity& entity, size_t newIndex) {
        auto it = std::find_if(m_Entities.begin(), m_Entities.end(), [&](const auto& e) { return e.get() == &entity; });
        if (it == m_Entities.end()) return;
        std::unique_ptr<Entity> owned = std::move(*it);
        m_Entities.erase(it);
        newIndex = std::min(newIndex, m_Entities.size());
        m_Entities.insert(m_Entities.begin() + (std::ptrdiff_t)newIndex, std::move(owned));
    }

    ScriptHost& Scene::GetScriptHost() {
        if (!m_Scripts) m_Scripts = std::make_unique<ScriptHost>(*this);
        return *m_Scripts;
    }

    bool Scene::TakePendingLoad(std::string& path) {
        if (m_PendingLoad.empty()) return false;
        path = std::move(m_PendingLoad);
        m_PendingLoad.clear();
        return true;
    }

    CameraComponent* Scene::GetPrimaryCamera() {
        CameraComponent* fallback = nullptr;
        for (auto& e : m_Entities) {
            if (!e->IsAlive()) continue;
            if (auto* cam = e->GetComponent<CameraComponent>()) {
                if (cam->primary) return cam;
                if (!fallback) fallback = cam;
            }
        }
        return fallback;
    }

    // Döngüler indeksle: güncelleme sırasında CreateEntity çağrılabilir.
    void Scene::FixedUpdate(float step) {
        if (m_Scripts) m_Scripts->BeginFixedStep(step);
        for (size_t i = 0; i < m_Entities.size(); ++i)
            if (m_Entities[i]->IsAlive()) m_Entities[i]->FixedUpdate(step);
        m_Physics.Step(*this, step);
        FlushDestroyed();
    }

    void Scene::Update(float dt) {
        if (m_Scripts) m_Scripts->BeginFrame(dt);
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

    void Scene::RenderUI() {
        for (size_t i = 0; i < m_Entities.size(); ++i)
            if (m_Entities[i]->IsAlive()) m_Entities[i]->RenderUI();
    }

    void Scene::RenderRuntime(int width, int height) {
        if (m_ViewSize == glm::vec2(0.0f)) m_ViewSize = { (float)width, (float)height };
        glm::vec4 background{ 0.10f, 0.11f, 0.15f, 1.0f };
        if (const CameraComponent* cam = GetPrimaryCamera()) {
            cam->Apply(m_RuntimeCamera, (float)width, (float)height);
            background = cam->background;
        } else {
            // Kamera yoksa: dünya orijini ortada, 1 birim = 1 piksel
            m_RuntimeCamera.SetViewportSize((float)width, (float)height);
            m_RuntimeCamera.SetZoom(1.0f);
            m_RuntimeCamera.SetPosition({ 0.0f, 0.0f });
            m_RuntimeCamera.SetRotation(0.0f);
        }
        Renderer2D::Clear(background);
        Renderer2D::BeginScene(m_RuntimeCamera);
        Render();
        Renderer2D::EndScene();

        Renderer2D::BeginScene(glm::ortho(0.0f, (float)width, (float)height, 0.0f));
        RenderUI();
        Renderer2D::EndScene();
    }

    void Scene::RenderDebug() {
        const SceneDebugDraw& dbg = GetDebugDraw();
        if (!dbg.colliders && !dbg.velocities && dbg.highlightedEntity == 0) return;

        for (Entity* e : m_RenderOrder) {
            const auto* rb = e->GetComponent<RigidBody2D>();
            const bool highlighted = e->GetID() == dbg.highlightedEntity;

            if (dbg.colliders || highlighted) {
                e->ForEachComponent([&](std::type_index, Component& c) {
                    const auto* col = dynamic_cast<const Collider2D*>(&c);
                    if (!col) return;
                    glm::vec4 color{ 0.6f, 0.6f, 0.6f, 0.9f };                 // statik
                    if (col->isTrigger) color = { 1.0f, 0.85f, 0.2f, 0.9f };
                    else if (rb && rb->type == BodyType::Kinematic) color = { 0.3f, 0.8f, 1.0f, 0.9f };
                    else if (rb && rb->type == BodyType::Dynamic) color = { 1.0f, 0.5f, 0.2f, 0.9f };
                    if (highlighted) color = { 1.0f, 1.0f, 1.0f, 1.0f };
                    DrawColliderOutline(*col, color, highlighted ? 3.0f : 1.5f);
                });
            }
            if (rb && dbg.velocities && glm::length(rb->velocity) > 0.0f) {
                // 0.25 sn sonra varacağı yer
                Renderer2D::DrawLine(e->position, e->position + rb->velocity * 0.25f, { 0.4f, 1.0f, 0.4f, 0.9f }, 2.0f);
            }
        }
    }

    void Scene::DrawColliderOutline(const Collider2D& col, const glm::vec4& color, float thicknessPx) {
        const Entity& e = col.GetEntity();
        const glm::vec2 center = col.GetCenter();
        if (const auto* box = dynamic_cast<const BoxCollider2D*>(&col)) {
            const glm::vec2 h = box->GetHalfExtents();
            const float r = glm::radians(e.rotation);
            const glm::vec2 ax{ std::cos(r), std::sin(r) }, ay{ -ax.y, ax.x };
            const glm::vec2 p[4] = {
                center - ax * h.x - ay * h.y, center + ax * h.x - ay * h.y,
                center + ax * h.x + ay * h.y, center - ax * h.x + ay * h.y,
            };
            for (int i = 0; i < 4; ++i) Renderer2D::DrawLine(p[i], p[(i + 1) % 4], color, thicknessPx);
        } else if (const auto* circle = dynamic_cast<const CircleCollider2D*>(&col)) {
            const float radius = circle->GetWorldRadius();
            constexpr int kSegments = 32;
            glm::vec2 prev = center + glm::vec2(radius, 0.0f);
            for (int i = 1; i <= kSegments; ++i) {
                const float a = glm::two_pi<float>() * (float)i / (float)kSegments;
                const glm::vec2 p = center + radius * glm::vec2(std::cos(a), std::sin(a));
                Renderer2D::DrawLine(prev, p, color, thicknessPx);
                prev = p;
            }
            // Dönüşü görmek için yarıçap çizgisi
            const float r = glm::radians(e.rotation);
            Renderer2D::DrawLine(center, center + radius * glm::vec2(std::cos(r), std::sin(r)), color, thicknessPx);
        }
    }

    void Scene::FlushDestroyed() {
        for (auto& e : m_Entities) {
            if (!e->IsPendingDestroy()) continue;
            m_Physics.OnEntityDestroyed(*e);
            m_ByID.erase(e->GetID());
        }
        m_Entities.erase(
            std::remove_if(m_Entities.begin(), m_Entities.end(),
                [](const std::unique_ptr<Entity>& e) { return e->IsPendingDestroy(); }),
            m_Entities.end());
    }
}
