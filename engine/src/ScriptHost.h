#pragma once
// Motor içi: Lua (sol2) durumu ve bağlamalar. Oyun kodu bu header'ı görmez.
#include <MME/ScriptComponent.h>

#include <sol/sol.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mme {

    class Entity;
    class Scene;

    // Lua'ya verilen entity tanıtıcısı: işaretçi değil (sahne, ID). Her erişimde
    // aranır; entity silindiyse anlaşılır bir Lua hatası üretir.
    struct EntityRef {
        Scene* scene = nullptr;
        uint32_t id = 0;

        Entity* Get() const;
        Entity& Expect() const;
        bool operator==(const EntityRef& o) const { return scene == o.scene && id == o.id; }
    };

    // Component tanıtıcısı: aynı şekilde her erişimde entity'den yeniden alınır
    template<typename T>
    struct ComponentRef {
        EntityRef owner;
        T& Expect() const;
    };

    struct ScriptComponent::Instance {
        sol::environment env;   // script'in fonksiyonları ve global'leri
        sol::table self;        // örnek durumu: self.entity, properties, kullanıcı alanları
    };

    // Inspector'ın gösterdiği, script'in properties tablosunda tanımlı bir değer
    struct ScriptProperty {
        enum class Type { Number, Bool, String, Vec2, Color };
        std::string name;
        Type type = Type::Number;
        double number = 0.0;
        bool boolean = false;
        std::string text;
        float vec[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    };

    class ScriptHost {
    public:
        explicit ScriptHost(Scene& scene);
        ~ScriptHost();
        ScriptHost(const ScriptHost&) = delete;
        ScriptHost& operator=(const ScriptHost&) = delete;

        sol::state& Lua() { return m_Lua; }
        Scene& GetScene() { return m_Scene; }

        // Scene::Update / FixedUpdate başında: Time tablosu ve dosya değişikliği kontrolü
        void BeginFrame(float dt);
        void BeginFixedStep(float step);

        // Script dosyasını okuyup env içinde çalıştırır. Hata mesajı döner (boş = başarılı).
        std::string RunFile(const std::string& relativePath, sol::environment& env);
        // Dosyanın sürümü: diskte her değiştiğinde artar
        uint64_t GetVersion(const std::string& relativePath);

        // Editör için: script'i çalıştırmadan (yalnızca üst düzey kod) properties tablosunu okur
        static bool ReadDeclaredProperties(const std::string& relativePath,
                                           std::vector<ScriptProperty>& out, std::string& error);

    private:
        void Bind();

        struct FileState {
            std::filesystem::file_time_type writeTime{};
            uint64_t version = 1;
        };

        Scene& m_Scene;
        sol::state m_Lua;
        std::unordered_map<std::string, FileState> m_Files;
        std::chrono::steady_clock::time_point m_LastPoll{};
        double m_Time = 0.0;
    };

    // Ortak: vec2 ve Color tiplerini bir Lua durumuna kaydeder
    void RegisterLuaMathTypes(sol::state& lua);

    // properties değerini Lua nesnesine / Lua nesnesinden çevirir
    sol::object PropertyToLua(sol::state_view lua, const ScriptProperty& p);
    bool PropertyFromLua(const std::string& name, const sol::object& value, ScriptProperty& out);
}
