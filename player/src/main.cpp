// Oynatıcı: editörde yapılmış bir projeyi çalıştırır.
//
//   Player.exe <proje klasörü> [--scene scenes/level2.json]
//
// Klasör verilmezse .exe'nin yanındaki "game" klasörü açılır (dışa aktarılmış oyun).
#include <MME/MME.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class PlayerLayer : public mme::Layer {
public:
    PlayerLayer(std::string scene, bool escapeQuits, std::string error)
        : Layer("Player"), m_ScenePath(std::move(scene)), m_EscapeQuits(escapeQuits), m_Error(std::move(error)) {}

    void OnAttach() override {
        if (m_Error.empty() && !mme::SceneSerializer::Load(m_Scene, m_ScenePath))
            m_Error = "Sahne yüklenemedi: " + m_ScenePath;
    }

    void OnFixedUpdate(float step) override {
        if (m_Error.empty()) m_Scene.FixedUpdate(step);
    }

    void OnUpdate(float dt) override {
        if (m_EscapeQuits && mme::Input::KeyPressed(mme::Key::Escape)) mme::Application::Get().Close();
        if (!m_Error.empty()) return;
        m_Scene.Update(dt);
        std::string next;
        if (m_Scene.TakePendingLoad(next)) {
            if (mme::SceneSerializer::Load(m_Scene, next)) m_ScenePath = next;
            else m_Error = "Sahne yüklenemedi: " + next;
        }
    }

    void OnRender() override {
        mme::Window& window = mme::Application::Get().GetWindow();
        const int w = window.GetWidth(), h = window.GetHeight();
        if (!m_Error.empty() || w <= 0 || h <= 0) return;
        m_Scene.SetViewRect({ 0.0f, 0.0f }, { (float)w, (float)h });
        m_Scene.RenderRuntime(w, h);
    }

    void OnUIRender() override {
        if (m_Error.empty()) return;
        mme::UI::Text(m_Error, { 24.0f, 24.0f }, 22.0f, { 1.0f, 0.45f, 0.4f, 1.0f });
        mme::UI::Text("Ayrıntılar konsolda.  Esc: çıkış", { 24.0f, 56.0f }, 18.0f, { 0.8f, 0.8f, 0.8f, 1.0f });
    }

private:
    mme::Scene m_Scene{ "main" };
    std::string m_ScenePath;
    bool m_EscapeQuits = true;
    std::string m_Error;
};

int main(int argc, char** argv) {
    std::string projectDir, sceneOverride;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--scene" && i + 1 < argc) sceneOverride = argv[++i];
        else if (projectDir.empty()) projectDir = arg;
    }
    // Application çalışma dizinini .exe klasörüne alır: göreli yolu önce mutlak yap
    if (!projectDir.empty()) projectDir = fs::absolute(projectDir).string();

    mme::Project project;
    std::string error;
    const bool loaded = !projectDir.empty() && mme::Project::Load(projectDir, project);

    mme::ApplicationSpec spec;
    spec.window.title = loaded ? project.name : "MakeMyEngine Player";
    spec.window.width = loaded ? project.windowWidth : 1280;
    spec.window.height = loaded ? project.windowHeight : 720;
    spec.uiFont = "fonts/FiraSans-Regular.ttf";
#ifdef NDEBUG
    spec.enableDebugTools = false;
#endif
    mme::Application app(spec);

    if (projectDir.empty()) {
        // Dışa aktarılmış oyun: <exe>/game/project.json
        projectDir = (fs::current_path() / "game").string();
        if (mme::Project::Load(projectDir, project)) {
            app.GetWindow().SetTitle(project.name);
        } else {
            error = "Proje bulunamadı. Kullanım: Player.exe <proje klasörü>";
        }
    } else if (!loaded) {
        error = "project.json okunamadı: " + projectDir;
    }

    if (error.empty()) mme::Assets::SetProjectDir(projectDir);
    app.PushLayer<PlayerLayer>(sceneOverride.empty() ? project.startScene : sceneOverride, project.escapeQuits, error);
    app.Run();
}
