#pragma once
#include <MME/MME.h>

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// Konsol satırları başka thread'lerden de gelebilir
std::mutex& EditorLogMutex();

// MakeMyEngine editörü: proje klasörünü açar, sahneleri görsel olarak düzenler,
// Lua script'li oyunu editör içinde oynatır ve oynatıcıyla (Player.exe) çalıştırır.
class EditorLayer : public mme::Layer {
public:
    explicit EditorLayer(std::string projectDir);

    void OnAttach() override;
    void OnDetach() override;
    void OnFixedUpdate(float step) override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    enum class GizmoMode { Move, Rotate, Scale };
    enum class GizmoHandle { None, X, Y, Free, Ring };

    struct Snapshot {
        std::string json;
        uint32_t selected = 0;
    };

    struct LogLine {
        mme::LogLevel level;
        std::string text;
    };

    // ------------------------------------------------ proje ve sahne (EditorLayer.cpp)
    bool OpenProject(const std::string& directory);
    void NewScene();
    bool OpenScene(const std::string& relativePath);
    bool SaveScene();
    void SaveSceneAs(const std::string& relativePath);
    void UpdateWindowTitle();

    // Sahne düzenlenen sahne mi, oynatılan kopya mı
    mme::Scene& ActiveScene() { return m_Runtime ? *m_Runtime : *m_Scene; }
    bool IsPlaying() const { return m_Runtime != nullptr; }
    mme::Entity* Selected() { return ActiveScene().FindEntity(m_Selected); }

    // ------------------------------------------------ geri alma
    void BeginEdit();          // değişiklikten hemen önce: anlık görüntüyü hazırla
    void CommitEdit();         // frame sonu: gerçekten değiştiyse geri alma yığınına it
    void Undo();
    void Redo();
    void Restore(const Snapshot& s);

    // ------------------------------------------------ oynatma
    void Play();
    void Stop();
    void LaunchPlayer();
    void ExportGame();

    // ------------------------------------------------ entity işlemleri
    mme::Entity& CreatePreset(const std::string& preset, const glm::vec2& position);
    void DeleteSelected();
    void DuplicateSelected();
    void FocusSelected();
    void FitCameraToGame();

    // ------------------------------------------------ paneller (EditorPanels.cpp)
    void BuildDefaultLayout(unsigned int dockspaceID);
    void DrawMenuBar();
    void DrawHierarchy();
    void DrawInspector();
    void DrawAssets();
    void DrawAssetFolder(const std::string& relativeDir);
    void DrawConsole();
    void DrawSceneSettings();
    void DrawProjectPicker();
    void HandleShortcuts();

    // ------------------------------------------------ viewport (EditorViewport.cpp)
    void DrawViewport();
    void DrawToolbar();
    void RenderEditView();
    void DrawGrid();
    void DrawSelectionOverlay();
    void DrawGizmo();
    void UpdateViewportInput();
    GizmoHandle HitGizmo(const glm::vec2& mouseWorld) const;
    uint32_t PickEntity(const glm::vec2& world) const;
    glm::vec2 ViewportToWorld(const glm::vec2& viewportPx) const;
    float PixelsToWorld(float px) const { return px * m_Camera.GetZoom(); }

    // ------------------------------------------------ durum
    mme::Project m_Project;
    std::string m_ProjectDirArg;
    bool m_HasProject = false;

    std::unique_ptr<mme::Scene> m_Scene;      // düzenlenen sahne
    std::unique_ptr<mme::Scene> m_Runtime;    // Play sırasında kopya
    std::string m_ScenePath;                  // projeye göre, ör. "scenes/main.json"
    bool m_Dirty = false;
    bool m_PlayPaused = false;
    bool m_StepOnce = false;
    uint32_t m_Selected = 0;

    std::vector<Snapshot> m_UndoStack, m_RedoStack;
    std::optional<Snapshot> m_PendingEdit;

    // Viewport
    std::unique_ptr<mme::Framebuffer> m_Framebuffer;
    mme::OrthoCamera m_Camera{ 1280.0f, 720.0f };
    glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };
    glm::vec2 m_ViewportScreenPos{ 0.0f };     // görüntünün ImGui ekran konumu
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;
    bool m_ShowGrid = true;
    bool m_ShowColliders = true;
    float m_GridSize = 32.0f;
    GizmoMode m_GizmoMode = GizmoMode::Move;
    GizmoHandle m_HotHandle = GizmoHandle::None;
    GizmoHandle m_DragHandle = GizmoHandle::None;
    glm::vec2 m_DragStartMouse{ 0.0f };
    glm::vec2 m_DragStartPos{ 0.0f }, m_DragStartScale{ 1.0f };
    float m_DragStartRot = 0.0f;
    bool m_Panning = false;
    int m_FitCameraFrames = 0;

    // Paneller
    std::deque<LogLine> m_Log;
    int m_LogSink = 0;
    bool m_LogShowInfo = true, m_LogShowWarn = true, m_LogShowError = true;
    bool m_LogScrollToBottom = false;
    std::string m_SaveAsPath;
    std::string m_NewAssetName;
    std::string m_NewProjectName = "YeniOyun";
    std::string m_AssetDragPath;              // sürüklenen asset (payload için kalıcı tampon)
    bool m_OpenSaveAsPopup = false;
    bool m_OpenProjectPopup = false;
};
