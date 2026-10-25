#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

namespace {
    constexpr const char* kEntityPayload = "MME_ENTITY";

    std::string Lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        return s;
    }

    bool IsImage(const fs::path& p) {
        const std::string e = Lower(p.extension().string());
        return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp" || e == ".tga";
    }
    bool IsPrefab(const fs::path& p) { return Lower(p.filename().string()).size() > 12 && Lower(p.string()).find(".prefab.json") != std::string::npos; }
    bool IsScene(const fs::path& p) { return Lower(p.extension().string()) == ".json" && !IsPrefab(p) && p.filename() != mme::Project::kFileName; }
    bool IsScript(const fs::path& p) { return Lower(p.extension().string()) == ".lua"; }

    // Göreli yolları her zaman '/' ile tut (sahne dosyaları platformdan bağımsız kalsın)
    std::string Generic(const fs::path& p) { return p.generic_string(); }

    void OpenWithSystem(const fs::path& p) {
#ifdef _WIN32
        ShellExecuteW(nullptr, L"open", p.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        (void)p;
#endif
    }

    const char* kScriptTemplate = R"(-- Inspector'da düzenlenebilen değerler (self.speed olarak erişilir)
properties = {
    speed = 200,
}

function OnStart(self)
end

function OnUpdate(self, dt)
    local move = Input.Axis("A", "D")
    self.entity.x = self.entity.x + move * self.speed * dt
end

function OnCollision(self, other, normal)
end
)";

    std::string ProjectsDir() {
#ifdef MME_PROJECTS_DIR
        if (fs::is_directory(MME_PROJECTS_DIR)) return MME_PROJECTS_DIR;
#endif
        return (fs::current_path() / "projects").string();
    }
}

// ---------------------------------------------------------------- menü
void EditorLayer::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;
    const bool playing = IsPlaying();

    if (ImGui::BeginMenu("Dosya")) {
        ImGui::BeginDisabled(!m_HasProject || playing);
        if (ImGui::MenuItem("Yeni sahne", "Ctrl+N")) NewScene();
        if (ImGui::MenuItem("Sahneyi kaydet", "Ctrl+S")) SaveScene();
        if (ImGui::MenuItem("Farklı kaydet...")) {
            m_SaveAsPath = m_ScenePath.empty() ? "scenes/" + m_Scene->GetName() + ".json" : m_ScenePath;
            m_OpenSaveAsPopup = true;
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Proje aç / oluştur...")) m_OpenProjectPopup = true;
        ImGui::BeginDisabled(!m_HasProject);
        if (ImGui::MenuItem("Proje klasörünü göster")) OpenWithSystem(m_Project.directory);
        if (ImGui::MenuItem("Oyunu dışa aktar (.exe)")) ExportGame();
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Çıkış")) mme::Application::Get().Close();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Düzen")) {
        if (ImGui::MenuItem("Geri al", "Ctrl+Z", false, !playing && !m_UndoStack.empty())) Undo();
        if (ImGui::MenuItem("Yinele", "Ctrl+Y", false, !playing && !m_RedoStack.empty())) Redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Çoğalt", "Ctrl+D", false, Selected() != nullptr)) DuplicateSelected();
        if (ImGui::MenuItem("Sil", "Del", false, Selected() != nullptr)) DeleteSelected();
        if (ImGui::MenuItem("Seçiliye odaklan", "F", false, Selected() != nullptr)) FocusSelected();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Oyun")) {
        if (ImGui::MenuItem(playing ? "Durdur" : "Oynat", "Ctrl+P", false, m_HasProject)) playing ? Stop() : Play();
        if (ImGui::MenuItem("Duraklat", nullptr, m_PlayPaused, playing)) m_PlayPaused = !m_PlayPaused;
        if (ImGui::MenuItem("Tek adım", nullptr, false, playing && m_PlayPaused)) m_StepOnce = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Oynatıcıda çalıştır", "F6", false, m_HasProject)) LaunchPlayer();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Görünüm")) {
        ImGui::MenuItem("Izgara", nullptr, &m_ShowGrid);
        ImGui::MenuItem("Collider'lar", nullptr, &m_ShowColliders);
        ImGui::EndMenu();
    }

    // Ortada oynatma düğmeleri
    const float buttons = 3.0f * 70.0f;
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), (ImGui::GetWindowWidth() - buttons) * 0.5f));
    ImGui::BeginDisabled(!m_HasProject);
    if (playing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.25f, 0.2f, 1.0f));
        if (ImGui::Button("Durdur", ImVec2(70, 0))) Stop();
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.3f, 1.0f));
        if (ImGui::Button("Oynat", ImVec2(70, 0))) Play();
        ImGui::PopStyleColor();
    }
    ImGui::BeginDisabled(!playing);
    if (ImGui::Button(m_PlayPaused ? "Devam" : "Duraklat", ImVec2(70, 0))) m_PlayPaused = !m_PlayPaused;
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_PlayPaused);
    if (ImGui::Button("Adım", ImVec2(50, 0))) m_StepOnce = true;
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    ImGui::EndMainMenuBar();

    // Farklı kaydet penceresi
    if (m_OpenSaveAsPopup) { ImGui::OpenPopup("Farklı kaydet"); m_OpenSaveAsPopup = false; }
    if (ImGui::BeginPopupModal("Farklı kaydet", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Proje klasörüne göre yol:");
        ImGui::SetNextItemWidth(320.0f);
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        const bool enter = ImGui::InputText("##path", &m_SaveAsPath, ImGuiInputTextFlags_EnterReturnsTrue);
        if ((ImGui::Button("Kaydet") || enter) && !m_SaveAsPath.empty()) {
            SaveSceneAs(m_SaveAsPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Vazgeç")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------- hiyerarşi
void EditorLayer::DrawHierarchy() {
    if (!ImGui::Begin("Hiyerarşi")) { ImGui::End(); return; }
    mme::Scene& scene = ActiveScene();

    if (ImGui::Button("+ Ekle")) ImGui::OpenPopup("add_entity");
    if (ImGui::BeginPopup("add_entity")) {
        for (const char* preset : { "Boş", "Sprite", "Kutu", "Top", "Zemin", "Tetik", "Kamera", "Parçacık" })
            if (ImGui::MenuItem(preset)) CreatePreset(preset == std::string("Boş") ? "Entity" : preset, m_Camera.GetPosition());
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    static std::string s_Filter;
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##filter", "Ara...", &s_Filter);
    const std::string filter = Lower(s_Filter);

    ImGui::BeginChild("##entities");
    const auto& entities = scene.GetEntities();
    int moveFrom = -1, moveTo = -1;
    for (size_t i = 0; i < entities.size(); ++i) {
        mme::Entity& e = *entities[i];
        if (e.IsPendingDestroy()) continue;
        if (!filter.empty() && Lower(e.GetName()).find(filter) == std::string::npos) continue;

        ImGui::PushID((int)e.GetID());
        if (!e.active) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        const char* icon = e.HasComponent<mme::CameraComponent>() ? "[K] "
                         : e.HasComponent<mme::ScriptComponent>() ? "[S] "
                         : e.HasComponent<mme::RigidBody2D>() ? "[F] " : "";
        const std::string label = std::string(icon) + e.GetName();
        if (ImGui::Selectable(label.c_str(), e.GetID() == m_Selected)) m_Selected = e.GetID();
        if (!e.active) ImGui::PopStyleColor();

        // Sürükle-bırak ile sırayı değiştir
        if (ImGui::BeginDragDropSource()) {
            const int index = (int)i;
            ImGui::SetDragDropPayload(kEntityPayload, &index, sizeof(index));
            ImGui::TextUnformatted(e.GetName().c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(kEntityPayload)) {
                moveFrom = *static_cast<const int*>(p->Data);
                moveTo = (int)i;
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem("ctx")) {
            m_Selected = e.GetID();
            if (ImGui::MenuItem("Çoğalt", "Ctrl+D")) DuplicateSelected();
            if (ImGui::MenuItem("Sil", "Del")) DeleteSelected();
            if (ImGui::MenuItem("Prefab olarak kaydet")) {
                std::string name = e.GetName();
                std::replace_if(name.begin(), name.end(), [](char c) { return c == ' ' || c == '/' || c == '\\'; }, '_');
                mme::SceneSerializer::SavePrefab(e, "prefabs/" + name + ".prefab.json");
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    if (moveFrom >= 0 && moveTo >= 0 && moveFrom != moveTo) {
        BeginEdit();
        scene.MoveEntity(*entities[(size_t)moveFrom], (size_t)moveTo);
    }
    // Boş alana tıklayınca seçimi kaldır
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        m_Selected = 0;
    ImGui::EndChild();
    ImGui::End();
}

// ---------------------------------------------------------------- inspector
void EditorLayer::DrawInspector() {
    if (!ImGui::Begin("Inspector")) { ImGui::End(); return; }
    if (mme::Entity* e = Selected()) {
        if (IsPlaying()) ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Oyun çalışıyor: değişiklikler Durdur'da geri alınır");
        mme::InspectorUI::Hooks hooks;
        hooks.beforeEdit = [this] { BeginEdit(); };
        mme::InspectorUI::DrawEntity(*e, hooks);
    } else {
        ImGui::TextDisabled("Bir entity seçin");
    }
    ImGui::End();
}

// ---------------------------------------------------------------- asset tarayıcı
void EditorLayer::DrawAssetFolder(const std::string& relativeDir) {
    const fs::path root(m_Project.directory);
    std::error_code ec;
    std::vector<fs::directory_entry> dirs, files;
    for (const auto& entry : fs::directory_iterator(root / relativeDir, ec)) {
        if (entry.is_directory(ec)) dirs.push_back(entry);
        else files.push_back(entry);
    }
    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) { return a.path().filename() < b.path().filename(); };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);

    for (const auto& d : dirs) {
        const std::string rel = Generic(fs::relative(d.path(), root, ec));
        const std::string name = d.path().filename().string();
        const bool open = ImGui::TreeNodeEx(rel.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s/", name.c_str());
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Yeni Lua script")) {
                fs::path p = d.path() / "yeni_script.lua";
                for (int n = 2; fs::exists(p, ec); ++n) p = d.path() / ("yeni_script" + std::to_string(n) + ".lua");
                std::ofstream(p, std::ios::binary) << kScriptTemplate;
                MME_INFO("[Editor] Olusturuldu: ", Generic(fs::relative(p, root, ec)));
            }
            if (ImGui::MenuItem("Klasörde göster")) OpenWithSystem(d.path());
            ImGui::EndPopup();
        }
        if (open) {
            DrawAssetFolder(rel);
            ImGui::TreePop();
        }
    }

    for (const auto& f : files) {
        const fs::path& p = f.path();
        const std::string rel = Generic(fs::relative(p, root, ec));
        const std::string name = p.filename().string();
        ImGui::PushID(rel.c_str());

        const char* tag = IsImage(p) ? "[R]" : IsPrefab(p) ? "[P]" : IsScene(p) ? "[Sahne]" : IsScript(p) ? "[Lua]" : "";
        const bool current = rel == m_ScenePath;
        ImGui::Selectable(("  " + std::string(tag) + " " + name).c_str(), current, ImGuiSelectableFlags_AllowDoubleClick);

        if (ImGui::IsItemHovered() && IsImage(p)) {
            ImGui::BeginTooltip();
            const mme::TextureID tex = mme::Assets::GetTexture(rel);
            const mme::TextureInfo info = mme::Assets::GetTextureInfo(tex);
            ImGui::Image((ImTextureID)(uintptr_t)tex, ImVec2(128.0f, info.width > 0 ? 128.0f * info.height / info.width : 128.0f),
                         ImVec2(0, 1), ImVec2(1, 0));
            ImGui::Text("%dx%d", info.width, info.height);
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (IsScene(p) && !IsPlaying()) {
                if (!m_Dirty || SaveScene() || m_ScenePath.empty()) OpenScene(rel);
            } else if (IsPrefab(p)) {
                BeginEdit();
                if (mme::Entity* e = mme::SceneSerializer::Instantiate(ActiveScene(), rel)) {
                    e->position = m_Camera.GetPosition();
                    m_Selected = e->GetID();
                }
            } else if (IsScript(p)) {
                OpenWithSystem(p);
            }
        }
        // Inspector'daki doku/script alanlarına ya da viewport'a sürüklenebilir
        if (ImGui::BeginDragDropSource()) {
            m_AssetDragPath = rel;
            ImGui::SetDragDropPayload(mme::InspectorUI::kAssetPayload, m_AssetDragPath.c_str(), m_AssetDragPath.size() + 1);
            ImGui::TextUnformatted(rel.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginPopupContextItem("ctx")) {
            if (ImGui::MenuItem("Sistemde aç")) OpenWithSystem(p);
            if (IsScene(p) && ImGui::MenuItem("Başlangıç sahnesi yap")) {
                m_Project.startScene = rel;
                m_Project.Save();
            }
            if (ImGui::MenuItem("Sil")) {
                fs::remove(p, ec);
                MME_INFO("[Editor] Silindi: ", rel);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
}

void EditorLayer::DrawAssets() {
    if (!ImGui::Begin("Dosyalar")) { ImGui::End(); return; }
    if (ImGui::Button("+ Script")) ImGui::OpenPopup("new_script");
    ImGui::SameLine();
    if (ImGui::Button("Klasörü aç")) OpenWithSystem(m_Project.directory);
    ImGui::SameLine();
    ImGui::TextDisabled("sürükle: inspector / sahne  -  çift tık: aç");

    if (ImGui::BeginPopup("new_script")) {
        ImGui::TextUnformatted("scripts/ klasöründe yeni script:");
        if (ImGui::IsWindowAppearing()) { m_NewAssetName = "oyuncu"; ImGui::SetKeyboardFocusHere(); }
        const bool enter = ImGui::InputText("##name", &m_NewAssetName, ImGuiInputTextFlags_EnterReturnsTrue);
        if ((ImGui::Button("Oluştur") || enter) && !m_NewAssetName.empty()) {
            std::error_code ec;
            const fs::path dir = fs::path(m_Project.directory) / "scripts";
            fs::create_directories(dir, ec);
            fs::path p = dir / (m_NewAssetName + (IsScript(m_NewAssetName) ? "" : ".lua"));
            if (!fs::exists(p, ec)) {
                std::ofstream(p, std::ios::binary) << kScriptTemplate;
                MME_INFO("[Editor] Script olusturuldu: scripts/", p.filename().string());
                // Seçili entity'ye hemen bağla
                if (mme::Entity* e = Selected(); e && !IsPlaying()) {
                    BeginEdit();
                    auto* sc = e->GetComponent<mme::ScriptComponent>();
                    if (!sc) sc = &e->AddComponent<mme::ScriptComponent>();
                    sc->script = "scripts/" + p.filename().string();
                }
            } else {
                MME_WARN("[Editor] Zaten var: ", p.string());
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::BeginChild("##tree");
    DrawAssetFolder("");
    ImGui::EndChild();
    ImGui::End();
}

// ---------------------------------------------------------------- konsol
void EditorLayer::DrawConsole() {
    if (!ImGui::Begin("Konsol")) { ImGui::End(); return; }
    std::lock_guard<std::mutex> lock(EditorLogMutex());
    if (ImGui::Button("Temizle")) m_Log.clear();
    ImGui::SameLine();
    ImGui::Checkbox("Bilgi", &m_LogShowInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Uyarı", &m_LogShowWarn);
    ImGui::SameLine();
    ImGui::Checkbox("Hata", &m_LogShowError);
    ImGui::Separator();

    ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    for (const LogLine& line : m_Log) {
        ImVec4 color(0.85f, 0.85f, 0.85f, 1.0f);
        switch (line.level) {
            case mme::LogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); if (!m_LogShowInfo) continue; break;
            case mme::LogLevel::Info:  if (!m_LogShowInfo) continue; break;
            case mme::LogLevel::Warn:  color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); if (!m_LogShowWarn) continue; break;
            case mme::LogLevel::Error: color = ImVec4(1.0f, 0.42f, 0.38f, 1.0f); if (!m_LogShowError) continue; break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(line.text.c_str());
        ImGui::PopStyleColor();
    }
    if (m_LogScrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 40.0f) ImGui::SetScrollHereY(1.0f);
    m_LogScrollToBottom = false;
    ImGui::EndChild();
    ImGui::End();
}

// ---------------------------------------------------------------- sahne ve proje ayarları
void EditorLayer::DrawSceneSettings() {
    if (!ImGui::Begin("Ayarlar")) { ImGui::End(); return; }
    mme::Scene& scene = ActiveScene();
    mme::InspectorUI::Hooks hooks;
    if (!IsPlaying()) hooks.beforeEdit = [this] { BeginEdit(); };
    auto track = [&](bool changed) { if (ImGui::IsItemActivated() && hooks.beforeEdit) hooks.beforeEdit(); return changed; };

    if (ImGui::CollapsingHeader("Sahne", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::string name = scene.GetName();
        if (track(ImGui::InputText("İsim", &name))) scene.SetName(name);
        mme::Physics2D& physics = scene.GetPhysics();
        track(ImGui::DragFloat2("Yerçekimi", &physics.gravity.x, 10.0f));
        track(ImGui::SliderInt("Fizik alt adımı", &physics.subSteps, 1, 8));
        const auto st = physics.GetStats();
        if (IsPlaying()) ImGui::TextDisabled("Gövde %u  Uyanık %u  Temas %u", st.bodies, st.awakeBodies, st.contacts);
    }
    if (ImGui::CollapsingHeader("Editör", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Izgara", &m_ShowGrid);
        ImGui::SameLine();
        ImGui::Checkbox("Collider'lar", &m_ShowColliders);
        ImGui::DragFloat("Izgara boyu", &m_GridSize, 1.0f, 4.0f, 512.0f);
        ImGui::TextDisabled("Ctrl basılıyken sürükleme ızgaraya/15°'ye oturur");
        const auto& rs = mme::Renderer2D::GetStats();
        ImGui::TextDisabled("%.0f FPS  draw %u  quad %u", 1.0f / std::max(mme::Application::Get().GetFrameTime(), 1e-4f), rs.drawCalls, rs.quadCount);
    }
    if (ImGui::CollapsingHeader("Proje")) {
        bool changed = false;
        changed |= ImGui::InputText("Oyun adı", &m_Project.name);
        if (ImGui::BeginCombo("Başlangıç sahnesi", m_Project.startScene.c_str())) {
            std::error_code ec;
            for (const auto& entry : fs::recursive_directory_iterator(m_Project.directory, ec)) {
                if (!IsScene(entry.path())) continue;
                const std::string rel = Generic(fs::relative(entry.path(), m_Project.directory, ec));
                if (ImGui::Selectable(rel.c_str(), rel == m_Project.startScene)) { m_Project.startScene = rel; changed = true; }
            }
            ImGui::EndCombo();
        }
        changed |= ImGui::DragInt("Pencere genişliği", &m_Project.windowWidth, 1.0f, 320, 3840);
        changed |= ImGui::DragInt("Pencere yüksekliği", &m_Project.windowHeight, 1.0f, 240, 2160);
        changed |= ImGui::Checkbox("Esc oyunu kapatır", &m_Project.escapeQuits);
        if (changed) m_Project.Save();
        ImGui::TextDisabled("%s", m_Project.directory.c_str());
    }
    ImGui::End();
}

// ---------------------------------------------------------------- proje seçici
void EditorLayer::DrawProjectPicker() {
    if (m_OpenProjectPopup) { ImGui::OpenPopup("Proje"); m_OpenProjectPopup = false; }
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Proje", nullptr, ImGuiWindowFlags_NoResize)) return;

    const std::string projectsDir = ProjectsDir();
    ImGui::TextDisabled("%s", projectsDir.c_str());
    ImGui::SeparatorText("Projeler");
    std::error_code ec;
    int count = 0;
    for (const auto& entry : fs::directory_iterator(projectsDir, ec)) {
        if (!entry.is_directory(ec) || !fs::exists(entry.path() / mme::Project::kFileName, ec)) continue;
        ++count;
        if (ImGui::Selectable(entry.path().filename().string().c_str(), false, ImGuiSelectableFlags_DontClosePopups) &&
            OpenProject(entry.path().string()))
            ImGui::CloseCurrentPopup();
    }
    if (count == 0) ImGui::TextDisabled("Henüz proje yok");

    ImGui::SeparatorText("Yeni proje");
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputText("##newproject", &m_NewProjectName);
    ImGui::SameLine();
    if (ImGui::Button("Oluştur") && !m_NewProjectName.empty()) {
        mme::Project created;
        const fs::path dir = fs::path(projectsDir) / m_NewProjectName;
        if (mme::Project::Create(dir.string(), m_NewProjectName, created) && OpenProject(dir.string()))
            ImGui::CloseCurrentPopup();
    }
    if (m_HasProject) {
        ImGui::Separator();
        if (ImGui::Button("Kapat")) ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// ---------------------------------------------------------------- kısayollar
void EditorLayer::HandleShortcuts() {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || !m_HasProject) return;
    const bool playing = IsPlaying();

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_P)) { playing ? Stop() : Play(); return; }
    if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) LaunchPlayer();
    // Oyun odaktayken tuşlar oyunundur
    if (playing && m_ViewportFocused) return;

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) SaveScene();
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N) && !playing) NewScene();
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z)) Undo();
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) Redo();
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_D)) DuplicateSelected();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) DeleteSelected();

    if (!io.KeyCtrl && (m_ViewportHovered || m_ViewportFocused)) {
        if (ImGui::IsKeyPressed(ImGuiKey_W, false)) m_GizmoMode = GizmoMode::Move;
        if (ImGui::IsKeyPressed(ImGuiKey_E, false)) m_GizmoMode = GizmoMode::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_GizmoMode = GizmoMode::Scale;
        if (ImGui::IsKeyPressed(ImGuiKey_F, false)) FocusSelected();
    }
}
