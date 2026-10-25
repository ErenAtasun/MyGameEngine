#include "DebugPanel.h"
#include <MME/Application.h>
#include <MME/Assets.h>
#include <MME/Audio.h>
#include <MME/ComponentRegistry.h>
#include <MME/Renderer2D.h>
#include <MME/Scene.h>
#include <MME/SceneSerializer.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <typeindex>
#include <vector>

namespace mme {

    DebugPanel::DebugPanel() : Layer("DebugPanel") {}

    void DebugPanel::OnUpdate(float /*dt*/) {
        m_FrameTimes[m_FrameIndex] = Application::Get().GetFrameTime() * 1000.0f;
        m_FrameIndex = (m_FrameIndex + 1) % (int)(sizeof(m_FrameTimes) / sizeof(m_FrameTimes[0]));
    }

    void DebugPanel::OnEvent(Event& e) {
        if (e.type == EventType::KeyPressed && e.key == Key::F1 && !e.repeat) {
            m_Visible = !m_Visible;
            e.handled = true;
        }
    }

    void DebugPanel::OnImGuiRender() {
        // Seçili entity'nin collider'ı panel açıkken dünyada vurgulanır
        Scene::GetDebugDraw().highlightedEntity = m_Visible ? m_Selected : 0;
        if (!m_Visible) return;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - 400.0f, vp->WorkPos.y + 20.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0f, 640.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("MakeMyEngine Debug (F1)", &m_Visible)) { ImGui::End(); return; }

        const auto& scenes = Scene::GetLiveScenes();
        Scene* scene = nullptr;
        if (!scenes.empty()) {
            m_SceneIndex = std::clamp(m_SceneIndex, 0, (int)scenes.size() - 1);
            scene = scenes[(size_t)m_SceneIndex];
        }

        DrawPerformance();
        DrawTime();
        DrawPhysics(scene);
        DrawAudio();

        if (scene) {
            if (scenes.size() > 1 && ImGui::BeginCombo("Sahne", scene->GetName().c_str())) {
                for (int i = 0; i < (int)scenes.size(); ++i)
                    if (ImGui::Selectable(scenes[(size_t)i]->GetName().c_str(), i == m_SceneIndex)) {
                        m_SceneIndex = i;
                        m_Selected = 0;
                    }
                ImGui::EndCombo();
            }
            DrawHierarchy(*scene);
            if (Entity* selected = scene->FindEntity(m_Selected))
                DrawInspector(*selected);
        } else {
            ImGui::TextDisabled("Canlı sahne yok");
        }

        ImGui::End();
    }

    void DebugPanel::DrawPerformance() {
        if (!ImGui::CollapsingHeader("Performans", ImGuiTreeNodeFlags_DefaultOpen)) return;
        const auto stats = Renderer2D::GetStats();
        const float ms = Application::Get().GetFrameTime() * 1000.0f;
        ImGui::Text("%.1f FPS  (%.2f ms)", ms > 0.0f ? 1000.0f / ms : 0.0f, ms);
        ImGui::Text("Draw call %u   Quad %u   Texture bind %u", stats.drawCalls, stats.quadCount, stats.textureBinds);
        ImGui::PlotLines("##frametimes", m_FrameTimes, (int)(sizeof(m_FrameTimes) / sizeof(m_FrameTimes[0])),
                         m_FrameIndex, "frame (ms)", 0.0f, 33.3f, ImVec2(-1.0f, 50.0f));
    }

    void DebugPanel::DrawTime() {
        if (!ImGui::CollapsingHeader("Zaman", ImGuiTreeNodeFlags_DefaultOpen)) return;
        Application& app = Application::Get();
        bool paused = app.IsPaused();
        if (ImGui::Checkbox("Duraklat", &paused)) app.SetPaused(paused);
        ImGui::SameLine();
        ImGui::BeginDisabled(!paused);
        if (ImGui::Button("Tek adım")) app.StepOneFrame();
        ImGui::EndDisabled();
        float scale = app.GetTimeScale();
        if (ImGui::SliderFloat("Zaman ölçeği", &scale, 0.0f, 3.0f, "%.2fx")) app.SetTimeScale(scale);
        static bool vsync = true;
        if (ImGui::Checkbox("VSync", &vsync)) app.GetWindow().SetVSync(vsync);
    }

    void DebugPanel::DrawPhysics(Scene* scene) {
        if (!ImGui::CollapsingHeader("Fizik")) return;
        SceneDebugDraw& dbg = Scene::GetDebugDraw();
        ImGui::Checkbox("Collider'ları çiz", &dbg.colliders);
        ImGui::SameLine();
        ImGui::Checkbox("Hız vektörleri", &dbg.velocities);
        if (scene) {
            Physics2D& physics = scene->GetPhysics();
            const auto& st = physics.GetStats();
            ImGui::Text("Gövde %u   Aday çift %u   Temas %u   Alt adım %u",
                        st.bodies, st.candidatePairs, st.contacts, st.substeps);
            ImGui::DragFloat2("Yerçekimi", &physics.gravity.x, 5.0f);
            ImGui::DragFloat("Hücre boyutu", &physics.cellSize, 1.0f, 16.0f, 1024.0f);
        }
    }

    void DebugPanel::DrawAudio() {
        if (!ImGui::CollapsingHeader("Ses")) return;
        if (!Audio::IsAvailable()) { ImGui::TextDisabled("Ses cihazı yok"); return; }
        float master = Audio::GetMasterVolume();
        if (ImGui::SliderFloat("Ana ses", &master, 0.0f, 1.0f)) Audio::SetMasterVolume(master);
        float sfx = Audio::GetGroupVolume(SoundGroup::SFX);
        if (ImGui::SliderFloat("Efektler", &sfx, 0.0f, 1.0f)) Audio::SetGroupVolume(SoundGroup::SFX, sfx);
        float music = Audio::GetGroupVolume(SoundGroup::Music);
        if (ImGui::SliderFloat("Müzik", &music, 0.0f, 1.0f)) Audio::SetGroupVolume(SoundGroup::Music, music);
        ImGui::Text("Çalan efekt: %d   Müzik: %s", Audio::GetActiveVoiceCount(), Audio::IsMusicPlaying() ? "açık" : "kapalı");
    }

    void DebugPanel::DrawHierarchy(Scene& scene) {
        if (!ImGui::CollapsingHeader("Hiyerarşi", ImGuiTreeNodeFlags_DefaultOpen)) return;

        if (m_SavePath.empty()) m_SavePath = "scenes/" + scene.GetName() + ".json";
        ImGui::InputText("##savepath", &m_SavePath);
        ImGui::SameLine();
        if (ImGui::Button("Kaydet")) SceneSerializer::Save(scene, m_SavePath);

        if (ImGui::Button("+ Entity")) m_Selected = scene.CreateEntity("Yeni Entity").GetID();
        ImGui::SameLine();
        Entity* selected = scene.FindEntity(m_Selected);
        ImGui::BeginDisabled(selected == nullptr);
        if (ImGui::Button("Sil") && selected) { selected->Destroy(); m_Selected = 0; }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("%zu entity", scene.GetEntityCount());

        ImGui::BeginChild("##hierarchy", ImVec2(0.0f, 160.0f), ImGuiChildFlags_Borders);
        for (auto& owned : scene.GetEntities()) {
            Entity& e = *owned;
            if (e.IsPendingDestroy()) continue;
            char label[160];
            std::snprintf(label, sizeof(label), "%s%s##%u", e.GetName().c_str(), e.active ? "" : " (pasif)", e.GetID());
            if (ImGui::Selectable(label, e.GetID() == m_Selected))
                m_Selected = e.GetID();
        }
        ImGui::EndChild();
    }

    void DebugPanel::DrawInspector(Entity& e) {
        if (!ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) return;
        ImGui::PushID((int)e.GetID());

        std::string name = e.GetName();
        if (ImGui::InputText("İsim", &name)) e.SetName(name);
        ImGui::Checkbox("Aktif", &e.active);
        ImGui::SameLine();
        ImGui::TextDisabled("ID %u", e.GetID());
        ImGui::DragFloat2("Konum", &e.position.x, 1.0f);
        ImGui::DragFloat("Rotasyon", &e.rotation, 1.0f);
        ImGui::DragFloat2("Ölçek", &e.scale.x, 0.01f);
        ImGui::DragFloat("zLayer", &e.zLayer, 0.1f);

        std::type_index toRemove = typeid(void);
        e.ForEachComponent([&](std::type_index type, Component& c) {
            const ComponentInfo* info = ComponentRegistry::Find(type);
            ImGui::PushID(type.name());
            if (!info) {
                ImGui::Separator();
                ImGui::TextDisabled("%s (kayıtsız)", type.name());
                ImGui::PopID();
                return;
            }
            const bool open = ImGui::CollapsingHeader(info->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
            if (ImGui::SmallButton("X")) toRemove = type;
            if (open)
                for (const FieldInfo& f : info->fields) DrawField(f, c);
            ImGui::PopID();
        });
        // Dolaşım bittikten sonra sil: döngü içinde vektör değişmesin
        if (toRemove != std::type_index(typeid(void))) e.RemoveComponent(toRemove);

        if (ImGui::BeginCombo("##add", "Component ekle…")) {
            for (const ComponentInfo& info : ComponentRegistry::GetAll()) {
                if (info.get(e)) continue;
                if (ImGui::Selectable(info.name.c_str())) info.add(e);
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }

    bool DebugPanel::DrawField(const FieldInfo& f, Component& c) {
        void* p = f.access(c);
        const char* label = f.name.c_str();
        switch (f.type) {
            case FieldType::Float:  return ImGui::DragFloat(label, static_cast<float*>(p), f.dragSpeed);
            case FieldType::Int:    return ImGui::DragInt(label, static_cast<int*>(p), std::max(f.dragSpeed, 0.1f));
            case FieldType::Bool:   return ImGui::Checkbox(label, static_cast<bool*>(p));
            case FieldType::Vec2:   return ImGui::DragFloat2(label, &static_cast<glm::vec2*>(p)->x, f.dragSpeed);
            case FieldType::Color:  return ImGui::ColorEdit4(label, &static_cast<glm::vec4*>(p)->x);
            case FieldType::String: return ImGui::InputText(label, static_cast<std::string*>(p));
            case FieldType::Enum: {
                int* v = static_cast<int*>(p);
                const char* preview = (*v >= 0 && *v < (int)f.enumNames.size()) ? f.enumNames[(size_t)*v].c_str() : "?";
                bool changed = false;
                if (ImGui::BeginCombo(label, preview)) {
                    for (int i = 0; i < (int)f.enumNames.size(); ++i)
                        if (ImGui::Selectable(f.enumNames[(size_t)i].c_str(), i == *v)) { *v = i; changed = true; }
                    ImGui::EndCombo();
                }
                return changed;
            }
            case FieldType::Texture: {
                const TextureID tex = *static_cast<TextureID*>(p);
                const std::string path = Assets::GetTexturePath(tex);
                ImGui::Text("%s: %s", label, tex == 0 ? "(yok)" : path.empty() ? "(bellekten)" : path.c_str());
                if (tex) {
                    const TextureInfo info = Assets::GetTextureInfo(tex);
                    const float w = 96.0f, h = info.width > 0 ? w * (float)info.height / (float)info.width : w;
                    // Dokularda v=0 altta; ImGui y'si aşağı baktığı için v çevrilir
                    ImGui::Image((ImTextureID)(uintptr_t)tex, ImVec2(w, h), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
                }
                return false;
            }
        }
        return false;
    }
}
