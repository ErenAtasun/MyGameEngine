#include <MME/MME.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

// Motorun dünya tarafını sınayan örnek: dokulu tilemap + kamera + animasyonlu sprite'lar.
//   WASD: kamera, tekerlek: zoom, sol tık: karoyu değiştir, sağ tık: sil,
//   F5: sahneyi kaydet, F9: yükle, F1: debug paneli, F3: istatistik
class TilemapLayer : public mme::Layer {
public:
    TilemapLayer() : Layer("TilemapLayer") {}

    void OnAttach() override {
        using namespace mme;

        TilemapDesc desc;
        desc.atlasTex = Assets::GetTexture("atlas.png", TextureFilter::Nearest);
        desc.atlasCols = 2;
        desc.atlasRows = 2;
        desc.tileW = desc.tileH = 64;
        m_Map.LoadCSV("maps/demo.csv", desc);

        BuildScene();

        const Window& window = Application::Get().GetWindow();
        m_Camera = std::make_unique<OrthoCameraController>((float)window.GetWidth(), (float)window.GetHeight());
        m_Camera->keyUp = Key::W;
        m_Camera->keyDown = Key::S;
        m_Camera->keyLeft = Key::A;
        m_Camera->keyRight = Key::D;
        m_Camera->GetCamera().SetPosition(m_Map.GetWorldSize() * 0.5f);
    }

    // Sahne koddan kurulur; F5/F9 ile aynı sahne JSON olarak kaydedilip geri yüklenebilir
    void BuildScene() {
        using namespace mme;
        m_Scene.Clear();

        // Haritanın ortasında, dokulu bir sprite entity
        m_Photo = Assets::GetTexture("texture.png");
        const TextureInfo info = Assets::GetTextureInfo(m_Photo);
        Entity& sign = m_Scene.CreateEntity("Tabela");
        sign.position = m_Map.GetWorldSize() * 0.5f;
        sign.zLayer = 1.0f;
        auto& sprite = sign.AddComponent<SpriteComponent>();
        sprite.texture = m_Photo;
        sprite.size = glm::vec2((float)info.width, (float)info.height) * 0.5f;

        // Dönen altınlar: 8 karelik sprite sheet, her biri farklı hızda
        const TextureID coinSheet = Assets::GetTexture("sprites/coin.png", TextureFilter::Nearest);
        const glm::vec2 spots[] = { { 320, 832 }, { 704, 960 }, { 1472, 896 }, { 1728, 448 }, { 448, 320 }, { 1216, 256 } };
        int index = 0;
        for (const glm::vec2& spot : spots) {
            Entity& coin = m_Scene.CreateEntity("Altin " + std::to_string(++index));
            coin.position = spot;
            coin.zLayer = 2.0f;
            auto& coinSprite = coin.AddComponent<SpriteComponent>();
            coinSprite.texture = coinSheet;
            coinSprite.size = { 48.0f, 48.0f };
            auto& anim = coin.AddComponent<SpriteAnimator>();
            anim.columns = 8;
            anim.rows = 1;
            anim.AddClip("spin", 0, 8, 10.0f + 2.0f * (float)index);
            anim.Play("spin");
        }
    }

    void OnUpdate(float dt) override {
        using namespace mme;
        if (Input::KeyPressed(Key::Escape)) Application::Get().Close();

        if (Input::KeyPressed(Key::F5)) {
            const bool ok = SceneSerializer::Save(m_Scene, kScenePath);
            Toast(ok ? "Sahne kaydedildi: " + std::string(kScenePath) : "Kaydetme başarısız");
        }
        if (Input::KeyPressed(Key::F9)) {
            const bool ok = SceneSerializer::Load(m_Scene, kScenePath);
            Toast(ok ? "Sahne yüklendi (" + std::to_string(m_Scene.GetEntityCount()) + " entity)" : "Önce F5 ile kaydedin");
        }

        m_Camera->OnUpdate(Application::Get().GetFrameTime()); // duraklatılsa da kamera gezilebilsin

        const glm::vec2 world = m_Camera->GetCamera().ScreenToWorld(Input::MousePosition());
        m_HasHover = m_Map.WorldToTile(world, m_HoverCol, m_HoverRow);
        if (m_HasHover) {
            const int id = m_Map.GetTile(m_HoverCol, m_HoverRow);
            if (Input::MousePressed(Mouse::Left))  m_Map.SetTile(m_HoverCol, m_HoverRow, (id + 1) % 4);
            if (Input::MousePressed(Mouse::Right)) m_Map.SetTile(m_HoverCol, m_HoverRow, -1);
        }

        m_Scene.Update(dt);
        m_ToastTime -= Application::Get().GetFrameTime();
    }

    void OnEvent(mme::Event& e) override {
        m_Camera->OnEvent(e);
    }

    void OnRender() override {
        using namespace mme;
        Renderer2D::BeginScene(m_Camera->GetCamera());
        m_Map.Draw(m_Camera->GetCamera());   // yalnızca görünen karolar
        m_Scene.Render();
        if (m_HasHover) {
            const glm::vec2 ts = m_Map.GetTileSize();
            Renderer2D::DrawQuad(m_Map.TileToWorld(m_HoverCol, m_HoverRow) + ts * 0.5f, ts, { 1.0f, 1.0f, 0.2f, 0.35f });
        }
        Renderer2D::EndScene();
    }

    void OnUIRender() override {
        using namespace mme;
        const Window& window = Application::Get().GetWindow();
        const float W = (float)window.GetWidth(), H = (float)window.GetHeight();
        const glm::vec4 white{ 1.0f }, panel{ 0.0f, 0.0f, 0.0f, 0.6f };

        const char* help = "WASD: kamera   Tekerlek: zoom   Sol tık: boya   Sağ tık: sil   F5: kaydet   F9: yükle   F1: debug";
        const glm::vec2 helpSize = UI::MeasureText(help, 20.0f);
        Renderer2D::DrawScreenRect({ (W - helpSize.x) * 0.5f - 12.0f, H - 44.0f }, helpSize + glm::vec2(24.0f, 16.0f), panel);
        UI::Text(help, { (W - helpSize.x) * 0.5f, H - 36.0f }, 20.0f, white);

        char info[96];
        if (m_HasHover)
            std::snprintf(info, sizeof(info), "Karo (%d, %d) = %d   zoom %.2f",
                m_HoverCol, m_HoverRow, m_Map.GetTile(m_HoverCol, m_HoverRow), m_Camera->GetCamera().GetZoom());
        else
            std::snprintf(info, sizeof(info), "Harita dışı   zoom %.2f", m_Camera->GetCamera().GetZoom());
        UI::Text(info, { W - UI::MeasureText(info, 20.0f).x - 16.0f, 14.0f }, 20.0f, white);

        // TTF font testi: Türkçe karakterler + yerleşik 5x7 fontun aynı yazıyı nasıl çizdiği
        const char* turkish = "Türkçe: ç ğ ı ö ş ü  Ç Ğ İ Ö Ş Ü";
        Renderer2D::DrawScreenRect({ 10.0f, 52.0f }, { 400.0f, 74.0f }, panel);
        UI::Text(turkish, { 20.0f, 58.0f }, 28.0f, { 1.0f, 0.9f, 0.5f, 1.0f });
        UI::Text(turkish, { 20.0f, 100.0f }, 14.0f, { 0.7f, 0.85f, 1.0f, 1.0f }, m_BuiltinFont.get());

        if (m_ToastTime > 0.0f) {
            const float alpha = std::min(1.0f, m_ToastTime * 2.0f);
            const glm::vec2 size = UI::MeasureText(m_Toast, 24.0f) + glm::vec2(32.0f, 20.0f);
            const glm::vec2 pos{ (W - size.x) * 0.5f, 24.0f };
            Renderer2D::DrawScreenRect(pos, size, { 0.1f, 0.4f, 0.2f, 0.85f * alpha });
            UI::TextCentered(m_Toast, pos + size * 0.5f, 24.0f, { 1.0f, 1.0f, 1.0f, alpha });
        }

        // UI uzayında dokulu dikdörtgen (yön testi: yazı düz okunmalı)
        Renderer2D::DrawScreenRect({ W - 216.0f, H - 170.0f }, { 200.0f, 100.0f }, white, m_Photo);
    }

private:
    void Toast(std::string text) { m_Toast = std::move(text); m_ToastTime = 2.5f; }

    static constexpr const char* kScenePath = "scenes/tilemap_demo.json";

    mme::Scene m_Scene{ "TilemapDemo" };
    mme::Tilemap m_Map;
    std::unique_ptr<mme::OrthoCameraController> m_Camera;
    mme::TextureID m_Photo = 0;
    std::shared_ptr<mme::Font> m_BuiltinFont = mme::Font::CreateBuiltin();

    bool m_HasHover = false;
    int m_HoverCol = 0, m_HoverRow = 0;
    std::string m_Toast;
    float m_ToastTime = 0.0f;
};

int main() {
    mme::ApplicationSpec spec;
    spec.window.title = "MakeMyEngine - Tilemap Demo";
    spec.clearColor = { 0.16f, 0.17f, 0.21f, 1.0f };
    spec.uiFont = "fonts/FiraSans-Regular.ttf";

    mme::Application app(spec);
    app.PushLayer<TilemapLayer>();
    app.Run();
    return 0;
}
