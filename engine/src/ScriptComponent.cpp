#include <MME/ScriptComponent.h>
#include <MME/Entity.h>
#include <MME/Log.h>
#include <MME/Scene.h>
#include "ScriptHost.h"

#include <nlohmann/json.hpp>

namespace mme {

    namespace {
        // Inspector'da kaydedilen değer (JSON) -> Lua değeri. Tipi script'teki
        // varsayılan belirler; uyuşmazsa varsayılan kalır.
        sol::object JsonToLua(sol::state_view lua, const nlohmann::json& j, const sol::object& def) {
            if (def.is<glm::vec2>() && j.is_array() && j.size() == 2)
                return sol::make_object(lua, glm::vec2(j[0].get<float>(), j[1].get<float>()));
            if (def.is<glm::vec4>() && j.is_array() && j.size() == 4)
                return sol::make_object(lua, glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()));
            if (def.get_type() == sol::type::number && j.is_number()) return sol::make_object(lua, j.get<double>());
            if (def.get_type() == sol::type::boolean && j.is_boolean()) return sol::make_object(lua, j.get<bool>());
            if (def.get_type() == sol::type::string && j.is_string()) return sol::make_object(lua, j.get<std::string>());
            return def;
        }
    }

    ScriptComponent::ScriptComponent() = default;
    ScriptComponent::~ScriptComponent() = default;

    bool ScriptComponent::IsRunning() const { return m_Started && !m_Failed; }

    void ScriptComponent::Fail(const std::string& message) {
        m_Failed = true;
        m_Error = message;
        MME_ERROR("[Lua] ", message);
    }

    // Script'in properties tablosunu self'e kopyalar; inspector'daki değerler üstüne yazılır.
    // onlyMissing: yeniden yüklemede mevcut durum korunur, yalnızca yeni alanlar eklenir.
    static void ApplyProperties(sol::state_view lua, ScriptComponent::Instance& inst, const std::string& overrides, bool onlyMissing) {
        sol::object props = inst.env["properties"];
        if (props.get_type() != sol::type::table) return;
        nlohmann::json values = overrides.empty() ? nlohmann::json::object() : nlohmann::json::parse(overrides, nullptr, false);
        if (!values.is_object()) values = nlohmann::json::object();

        for (auto& [k, v] : props.as<sol::table>()) {
            if (k.get_type() != sol::type::string) continue;
            const std::string key = k.as<std::string>();
            if (onlyMissing && inst.self[key].get_type() != sol::type::lua_nil) continue;
            const auto it = values.find(key);
            inst.self[key] = it != values.end() ? JsonToLua(lua, *it, v) : v;
        }
    }

    bool ScriptComponent::EnsureStarted() {
        if (m_Started && script != m_LoadedScript) {
            // Script yolu değişti: baştan başla
            m_Instance.reset();
            m_Started = false;
            m_Failed = false;
        }
        if (m_Started || m_Failed) return m_Started && !m_Failed;
        if (script.empty()) return false;

        Entity& entity = GetEntity();
        Scene& scene = entity.GetScene();
        ScriptHost& host = scene.GetScriptHost();
        sol::state& lua = host.Lua();

        m_LoadedScript = script;
        m_LoadedVersion = host.GetVersion(script);
        m_Instance = std::make_unique<Instance>();
        m_Instance->env = sol::environment(lua, sol::create, lua.globals());

        const std::string error = host.RunFile(script, m_Instance->env);
        if (!error.empty()) {
            m_Started = true; // dosya değişince CheckReload yeniden dener
            Fail(error);
            return false;
        }

        m_Instance->self = lua.create_table();
        m_Instance->self["entity"] = EntityRef{ &scene, entity.GetID() };
        // self:Fonksiyon() çağrıları script'in fonksiyonlarını bulsun
        sol::table meta = lua.create_table();
        meta["__index"] = m_Instance->env;
        m_Instance->self[sol::metatable_key] = meta;
        ApplyProperties(lua, *m_Instance, properties, false);

        m_Started = true;
        m_Failed = false;
        m_Error.clear();

        sol::object onStart = m_Instance->env["OnStart"];
        if (onStart.get_type() == sol::type::function) {
            sol::protected_function fn = onStart;
            sol::protected_function_result r = fn(m_Instance->self);
            if (!r.valid()) { sol::error e = r; Fail(e.what()); return false; }
        }
        return true;
    }

    void ScriptComponent::CheckReload() {
        if (!m_Started || !m_Instance || script != m_LoadedScript) return;
        ScriptHost& host = GetEntity().GetScene().GetScriptHost();
        const uint64_t version = host.GetVersion(script);
        if (version == m_LoadedVersion) return;
        m_LoadedVersion = version;

        const bool neverRan = !m_Instance->self.valid();
        if (neverRan) {
            // İlk yükleme başarısızdı: sıfırdan başlat
            m_Instance.reset();
            m_Started = false;
            m_Failed = false;
            EnsureStarted();
            if (IsRunning()) MME_INFO("[Lua] Yeniden yuklendi: ", script);
            return;
        }

        // Fonksiyonlar aynı env'de yeniden tanımlanır; self'teki durum korunur
        const std::string error = host.RunFile(script, m_Instance->env);
        if (!error.empty()) { Fail(error); return; }
        ApplyProperties(host.Lua(), *m_Instance, properties, true);
        m_Failed = false;
        m_Error.clear();
        MME_INFO("[Lua] Yeniden yuklendi: ", script);

        sol::object onReload = m_Instance->env["OnReload"];
        if (onReload.get_type() == sol::type::function) {
            sol::protected_function fn = onReload;
            sol::protected_function_result r = fn(m_Instance->self);
            if (!r.valid()) { sol::error e = r; Fail(e.what()); }
        }
    }

    namespace {
        // Script'te tanımlıysa fonksiyonu çağırır; hata mesajı döner (boş = sorun yok)
        template<typename... Args>
        std::string CallHook(ScriptComponent::Instance& inst, const char* name, Args&&... args) {
            sol::object f = inst.env[name];
            if (f.get_type() != sol::type::function) return {};
            sol::protected_function fn = f;
            sol::protected_function_result r = fn(inst.self, std::forward<Args>(args)...);
            if (r.valid()) return {};
            sol::error e = r;
            return e.what();
        }
    }

    void ScriptComponent::OnFixedUpdate(float step) {
        if (!EnsureStarted()) return;
        if (auto err = CallHook(*m_Instance, "OnFixedUpdate", step); !err.empty()) Fail(err);
    }

    void ScriptComponent::OnUpdate(float dt) {
        CheckReload();
        if (!EnsureStarted()) return;
        if (auto err = CallHook(*m_Instance, "OnUpdate", dt); !err.empty()) Fail(err);
    }

    void ScriptComponent::OnUIRender() {
        if (!IsRunning()) return;
        if (auto err = CallHook(*m_Instance, "OnUI"); !err.empty()) Fail(err);
    }

    void ScriptComponent::OnCollisionEnter(const Collision& collision) {
        if (!IsRunning() || !collision.other) return;
        if (auto err = CallHook(*m_Instance, "OnCollision", EntityRef{ &GetEntity().GetScene(), collision.other->GetID() }, collision.normal); !err.empty()) Fail(err);
    }

    void ScriptComponent::OnTriggerEnter(Entity& other) {
        if (!IsRunning()) return;
        if (auto err = CallHook(*m_Instance, "OnTrigger", EntityRef{ &GetEntity().GetScene(), other.GetID() }); !err.empty()) Fail(err);
    }

    void ScriptComponent::OnDestroy() {
        // Sahne tümden temizlenirken diğer entity'ler yarı yıkılmış olabilir: çağırma
        if (IsRunning() && !GetEntity().GetScene().IsClearing())
            if (auto err = CallHook(*m_Instance, "OnDestroy"); !err.empty()) Fail(err);
        m_Instance.reset();
    }

}
