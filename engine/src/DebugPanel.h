#pragma once
#include <MME/Layer.h>

#include <cstdint>
#include <string>

namespace mme {

    class Component;
    class Entity;
    class Scene;
    struct FieldInfo;

    // F1 ile açılan ImGui paneli: performans, zaman kontrolü, fizik, ses,
    // sahne hiyerarşisi ve ComponentRegistry tabanlı inspector.
    class DebugPanel : public Layer {
    public:
        DebugPanel();

        void OnUpdate(float dt) override;
        void OnEvent(Event& e) override;
        void OnImGuiRender() override;

    private:
        void DrawPerformance();
        void DrawTime();
        void DrawPhysics(Scene* scene);
        void DrawAudio();
        void DrawHierarchy(Scene& scene);
        void DrawInspector(Entity& entity);
        static bool DrawField(const FieldInfo& field, Component& component);

        bool m_Visible = false;
        int m_SceneIndex = 0;
        uint32_t m_Selected = 0;      // seçili entity ID'si
        std::string m_SavePath;
        float m_FrameTimes[120] = {};
        int m_FrameIndex = 0;
    };
}
