#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>  // DockBuilder: ilk açılıştaki pencere düzeni

#include <algorithm>
#include <filesystem>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
    constexpr size_t kMaxUndo = 200;
    constexpr size_t kMaxLogLines = 2000;

    fs::path ExecutableDir() {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (len > 0 && len < MAX_PATH) return fs::path(path).parent_path();
#endif
        return fs::current_path();
    }
}

std::mutex& EditorLogMutex() {
    static std::mutex s_Mutex;  // miniaudio vb. başka thread'den log atabilir
    return s_Mutex;
}

EditorLayer::EditorLayer(std::string projectDir)
    : Layer("Editor"), m_ProjectDirArg(std::move(projectDir)) {}

// ---------------------------------------------------------------- yaşam döngüsü
void EditorLayer::OnAttach() {
    m_LogSink = mme::AddLogSink([this](mme::LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(EditorLogMutex());
        m_Log.push_back({ level, message });
        if (m_Log.size() > kMaxLogLines) m_Log.pop_front();
        m_LogScrollToBottom = true;
    });

    m_Framebuffer = std::make_unique<mme::Framebuffer>(1280, 720);
    m_Scene = std::make_unique<mme::Scene>("main");

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.0f);

    if (!m_ProjectDirArg.empty() && OpenProject(m_ProjectDirArg)) return;
    m_OpenProjectPopup = true;
}

void EditorLayer::OnDetach() {
    m_Runtime.reset();
    m_Scene.reset();
    m_Framebuffer.reset();
    mme::RemoveLogSink(m_LogSink);
}

void EditorLayer::OnFixedUpdate(float step) {
    if (!m_Runtime) return;
    if (m_PlayPaused && !m_StepOnce) return;
    m_Runtime->FixedUpdate(step);
}

void EditorLayer::OnUpdate(float dt) {
    mme::Application& app = mme::Application::Get();
    if (m_Runtime) {
        if (!m_PlayPaused || m_StepOnce) m_Runtime->Update(m_StepOnce ? mme::Application::Get().GetSpec().fixedTimestep : dt);
        m_StepOnce = false;

        std::string next;
        if (m_Runtime->TakePendingLoad(next)) {
            MME_INFO("[Editor] Oyun sahne yukluyor: ", next);
            mme::SceneSerializer::Load(*m_Runtime, next);
        }
    }
    // Oyun görünümü odaktayken oyun, fare viewport penceresinin üstünde olsa da girdiyi alır
    app.SetImGuiInputCapture(!(m_Runtime && m_ViewportFocused));
}

void EditorLayer::OnRender() {
    if (!m_HasProject || !m_Framebuffer) return;
    const int w = std::max(1, (int)m_ViewportSize.x);
    const int h = std::max(1, (int)m_ViewportSize.y);
    m_Framebuffer->Resize(w, h);
    m_Framebuffer->Bind();
    if (m_Runtime) {
        // Script'lerin fare konumu: görüntünün pencere içindeki yeri (framebuffer pikseli)
        const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
        const ImVec2 origin = ImGui::GetMainViewport()->Pos;
        m_Runtime->SetViewRect({ (m_ViewportScreenPos.x - origin.x) * scale.x, (m_ViewportScreenPos.y - origin.y) * scale.y },
                               { (float)w, (float)h });
        mme::Scene::GetDebugDraw() = {};   // oyun görünümü oyundaki gibi: collider çizimi yok
        m_Runtime->RenderRuntime(w, h);
    } else {
        RenderEditView();
    }
    m_Framebuffer->Unbind();
}

void EditorLayer::OnImGuiRender() {
    DrawMenuBar();

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    BuildDefaultLayout(dockspace);

    if (m_HasProject) {
        DrawViewport();
        DrawHierarchy();
        DrawInspector();
        DrawAssets();
        DrawSceneSettings();
    }
    DrawConsole();
    DrawProjectPicker();
    HandleShortcuts();
    CommitEdit();
    UpdateWindowTitle();
}

void EditorLayer::BuildDefaultLayout(unsigned int dockspaceID) {
    // imgui.ini'de kayıtlı bir düzen varsa ona dokunma
    const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID);
    if (node && node->IsSplitNode()) return;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(dockspaceID);
    ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceID, vp->WorkSize);

    ImGuiID center = dockspaceID;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.17f, nullptr, &center);
    ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.27f, nullptr, &center);
    const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.32f, nullptr, &right);

    ImGui::DockBuilderDockWindow("Hiyerarşi", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Ayarlar", rightBottom);
    ImGui::DockBuilderDockWindow("Konsol", bottom);
    ImGui::DockBuilderDockWindow("Dosyalar", bottom);
    ImGui::DockBuilderDockWindow("Sahne", center);
    ImGui::DockBuilderFinish(dockspaceID);
}

// ---------------------------------------------------------------- proje ve sahne
bool EditorLayer::OpenProject(const std::string& directory) {
    mme::Project project;
    if (!mme::Project::Load(directory, project)) return false;
    Stop();
    m_Project = project;
    m_Project.directory = fs::absolute(directory).lexically_normal().string();
    mme::Assets::SetProjectDir(m_Project.directory);
    m_HasProject = true;
    m_OpenProjectPopup = false;

    std::error_code ec;
    if (!m_Project.startScene.empty() && fs::exists(fs::path(m_Project.directory) / m_Project.startScene, ec))
        OpenScene(m_Project.startScene);
    else
        NewScene();
    MME_INFO("[Editor] Proje acildi: ", m_Project.name);
    return true;
}

void EditorLayer::NewScene() {
    Stop();
    m_Scene->Clear();
    m_Scene->SetName("yeni");
    m_Scene->GetPhysics().gravity = { 0.0f, -1500.0f };
    mme::Entity& cam = m_Scene->CreateEntity("Kamera");
    cam.AddComponent<mme::CameraComponent>();
    m_ScenePath.clear();
    m_UndoStack.clear();
    m_RedoStack.clear();
    m_PendingEdit.reset();
    m_Selected = 0;
    m_Dirty = false;
    m_Camera.SetPosition({ 0.0f, 0.0f });
    m_Camera.SetZoom(1.0f);
}

bool EditorLayer::OpenScene(const std::string& relativePath) {
    Stop();
    if (!mme::SceneSerializer::Load(*m_Scene, relativePath)) return false;
    m_ScenePath = relativePath;
    m_UndoStack.clear();
    m_RedoStack.clear();
    m_PendingEdit.reset();
    m_Selected = 0;
    m_Dirty = false;
    // Viewport boyutu belli olunca (DrawViewport) oyun karesine sığdır. İlk açılışta dock
    // düzeni birkaç frame'de oturduğu için birkaç kez uygulanır.
    m_FitCameraFrames = 3;
    return true;
}

bool EditorLayer::SaveScene() {
    if (IsPlaying()) {
        MME_WARN("[Editor] Oyun calisirken kaydedilemez: once durdurun");
        return false;
    }
    if (m_ScenePath.empty()) {
        m_SaveAsPath = "scenes/" + m_Scene->GetName() + ".json";
        m_OpenSaveAsPopup = true;
        return false;
    }
    m_Scene->FlushDestroyed();
    if (!mme::SceneSerializer::Save(*m_Scene, m_ScenePath)) return false;
    m_Dirty = false;
    return true;
}

void EditorLayer::SaveSceneAs(const std::string& relativePath) {
    std::string path = relativePath;
    if (fs::path(path).extension() != ".json") path += ".json";
    m_ScenePath = path;
    m_Scene->SetName(fs::path(path).stem().string());
    SaveScene();
}

void EditorLayer::UpdateWindowTitle() {
    std::string title = "MakeMyEngine Editör";
    if (m_HasProject) {
        title += " - " + m_Project.name + " - " + (m_ScenePath.empty() ? std::string("(kaydedilmemiş sahne)") : m_ScenePath);
        if (m_Dirty) title += " *";
        if (IsPlaying()) title += m_PlayPaused ? "  [DURAKLATILDI]" : "  [OYNATILIYOR]";
    }
    static std::string s_Last;
    if (title != s_Last) {
        mme::Application::Get().GetWindow().SetTitle(title);
        s_Last = title;
    }
}

// ---------------------------------------------------------------- geri alma
void EditorLayer::BeginEdit() {
    if (IsPlaying() || m_PendingEdit) return;
    m_PendingEdit = Snapshot{ mme::SceneSerializer::ToJson(*m_Scene), m_Selected };
}

void EditorLayer::CommitEdit() {
    if (!m_PendingEdit) return;
    // Sürükleme sürerken bekle: tek bir sürükleme tek bir geri alma adımı olsun
    if (ImGui::IsAnyItemActive() || m_DragHandle != GizmoHandle::None) return;
    m_Scene->FlushDestroyed();
    const std::string now = mme::SceneSerializer::ToJson(*m_Scene);
    if (now != m_PendingEdit->json) {
        m_UndoStack.push_back(std::move(*m_PendingEdit));
        if (m_UndoStack.size() > kMaxUndo) m_UndoStack.erase(m_UndoStack.begin());
        m_RedoStack.clear();
        m_Dirty = true;
    }
    m_PendingEdit.reset();
}

void EditorLayer::Restore(const Snapshot& s) {
    mme::SceneSerializer::FromJson(*m_Scene, s.json);
    m_Selected = m_Scene->FindEntity(s.selected) ? s.selected : 0;
    m_Dirty = true;
}

void EditorLayer::Undo() {
    if (IsPlaying() || m_UndoStack.empty()) return;
    m_PendingEdit.reset();
    m_RedoStack.push_back({ mme::SceneSerializer::ToJson(*m_Scene), m_Selected });
    Snapshot s = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();
    Restore(s);
}

void EditorLayer::Redo() {
    if (IsPlaying() || m_RedoStack.empty()) return;
    m_PendingEdit.reset();
    m_UndoStack.push_back({ mme::SceneSerializer::ToJson(*m_Scene), m_Selected });
    Snapshot s = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();
    Restore(s);
}

// ---------------------------------------------------------------- oynatma
void EditorLayer::Play() {
    if (IsPlaying() || !m_HasProject) return;
    CommitEdit();
    m_Scene->FlushDestroyed();
    m_Runtime = std::make_unique<mme::Scene>(m_Scene->GetName());
    // Düzenlenen sahneye dokunmadan kopya: Stop'ta her şey eski haline döner
    mme::SceneSerializer::FromJson(*m_Runtime, mme::SceneSerializer::ToJson(*m_Scene));
    m_PlayPaused = false;
    ImGui::SetWindowFocus("Sahne");
    MME_INFO("[Editor] Oynatiliyor: ", m_Scene->GetName());
}

void EditorLayer::Stop() {
    if (!m_Runtime) return;
    m_Runtime.reset();
    m_PlayPaused = false;
    mme::Audio::StopMusic();
    mme::Application::Get().SetImGuiInputCapture(true);
    if (!m_Scene->FindEntity(m_Selected)) m_Selected = 0;
    MME_INFO("[Editor] Durduruldu");
}

void EditorLayer::LaunchPlayer() {
    if (!m_HasProject) return;
    if (!IsPlaying() && m_Dirty && !m_ScenePath.empty()) SaveScene();
    const fs::path player = ExecutableDir() / "Player.exe";
    if (!fs::exists(player)) {
        MME_ERROR("[Editor] Player.exe bulunamadi: ", player.string());
        return;
    }
#ifdef _WIN32
    std::wstring cmd = L"\"" + player.wstring() + L"\" \"" + fs::path(m_Project.directory).wstring() + L"\"";
    if (!m_ScenePath.empty()) cmd += L" --scene \"" + fs::path(m_ScenePath).wstring() + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                       player.parent_path().wstring().c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        MME_INFO("[Editor] Oynatici baslatildi");
    } else {
        MME_ERROR("[Editor] Oynatici baslatilamadi (", (unsigned)GetLastError(), ")");
    }
#endif
}

void EditorLayer::ExportGame() {
    if (!m_HasProject) return;
    if (m_Dirty && !m_ScenePath.empty()) SaveScene();
    const fs::path exeDir = ExecutableDir();
    const fs::path player = exeDir / "Player.exe";
    const fs::path out = fs::path(m_Project.directory).parent_path() / (fs::path(m_Project.directory).filename().string() + "_Build");
    std::error_code ec;
    fs::remove_all(out, ec);
    fs::create_directories(out, ec);
    if (!fs::exists(player) || ec) {
        MME_ERROR("[Editor] Disa aktarilamadi (Player.exe: ", player.string(), ")");
        return;
    }

    // Oynatıcı + çalışma zamanı DLL'leri + motor asset'leri (fontlar) + proje
    fs::copy_file(player, out / (m_Project.name + ".exe"), fs::copy_options::overwrite_existing, ec);
    for (const auto& entry : fs::directory_iterator(exeDir, ec))
        if (entry.path().extension() == ".dll") fs::copy_file(entry.path(), out / entry.path().filename(), fs::copy_options::overwrite_existing, ec);
    fs::copy(mme::Assets::GetBaseDir(), out / "assets", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    fs::copy(m_Project.directory, out / "game", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) MME_ERROR("[Editor] Kopyalama hatasi: ", ec.message());
    else MME_INFO("[Editor] Oyun disa aktarildi: ", out.string());
}

// ---------------------------------------------------------------- entity işlemleri
mme::Entity& EditorLayer::CreatePreset(const std::string& preset, const glm::vec2& position) {
    BeginEdit();
    mme::Scene& scene = ActiveScene();
    mme::Entity& e = scene.CreateEntity(preset);
    e.position = position;

    auto sprite = [&](glm::vec2 size, glm::vec4 tint) -> mme::SpriteComponent& {
        auto& s = e.AddComponent<mme::SpriteComponent>();
        s.size = size;
        s.tint = tint;
        return s;
    };
    if (preset == "Sprite") {
        sprite({ 64, 64 }, glm::vec4(1.0f));
    } else if (preset == "Kutu") {
        sprite({ 64, 64 }, { 0.95f, 0.6f, 0.25f, 1.0f });
        e.AddComponent<mme::BoxCollider2D>().size = { 64, 64 };
        e.AddComponent<mme::RigidBody2D>();
    } else if (preset == "Top") {
        auto& s = sprite({ 48, 48 }, { 0.35f, 0.75f, 1.0f, 1.0f });
        s.texture = mme::Assets::GetTexture("sprites/circle.png");
        auto& c = e.AddComponent<mme::CircleCollider2D>();
        c.radius = 24.0f;
        c.restitution = 0.5f;
        e.AddComponent<mme::RigidBody2D>();
    } else if (preset == "Zemin") {
        sprite({ 512, 32 }, { 0.35f, 0.38f, 0.45f, 1.0f });
        e.AddComponent<mme::BoxCollider2D>().size = { 512, 32 };
    } else if (preset == "Tetik") {
        sprite({ 96, 96 }, { 1.0f, 0.85f, 0.2f, 0.25f });
        auto& c = e.AddComponent<mme::BoxCollider2D>();
        c.size = { 96, 96 };
        c.isTrigger = true;
    } else if (preset == "Kamera") {
        e.AddComponent<mme::CameraComponent>();
    } else if (preset == "Parçacık") {
        e.AddComponent<mme::ParticleEmitter>();
    }
    m_Selected = e.GetID();
    return e;
}

void EditorLayer::DeleteSelected() {
    mme::Entity* e = Selected();
    if (!e) return;
    BeginEdit();
    e->Destroy();
    if (!IsPlaying()) m_Scene->FlushDestroyed();
    m_Selected = 0;
}

void EditorLayer::DuplicateSelected() {
    mme::Entity* e = Selected();
    if (!e) return;
    BeginEdit();
    mme::Scene& scene = ActiveScene();
    if (mme::Entity* copy = mme::SceneSerializer::EntityFromJson(scene, mme::SceneSerializer::EntityToJson(*e))) {
        copy->position += glm::vec2(m_GridSize);
        m_Selected = copy->GetID();
    }
}

void EditorLayer::FocusSelected() {
    if (mme::Entity* e = Selected()) m_Camera.SetPosition(e->position);
}
