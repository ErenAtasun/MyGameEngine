#include "ScriptHost.h"
#include <MME/Assets.h>
#include <MME/Audio.h>
#include <MME/CameraComponent.h>
#include <MME/Collider2D.h>
#include <MME/ComponentRegistry.h>
#include <MME/Input.h>
#include <MME/Log.h>
#include <MME/ParticleEmitter.h>
#include <MME/Renderer2D.h>
#include <MME/RigidBody2D.h>
#include <MME/Scene.h>
#include <MME/SceneSerializer.h>
#include <MME/SpriteAnimator.h>
#include <MME/SpriteComponent.h>
#include <MME/UI.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace mme {

    // ---------------------------------------------------------------- tanıtıcılar
    Entity* EntityRef::Get() const { return scene ? scene->FindEntity(id) : nullptr; }

    Entity& EntityRef::Expect() const {
        Entity* e = Get();
        if (!e) throw std::runtime_error("entity artik yok (silinmis), id " + std::to_string(id));
        return *e;
    }

    template<typename T>
    T& ComponentRef<T>::Expect() const {
        Entity& e = owner.Expect();
        T* c = e.GetComponent<T>();
        if (!c) throw std::runtime_error("'" + e.GetName() + "' uzerinde bu component artik yok");
        return *c;
    }

    namespace {
        // ------------------------------------------------------------ yardımcılar
        std::string ReadText(const std::string& relativePath, std::string& error) {
            std::ifstream in(Assets::GetPath(relativePath), std::ios::binary);
            if (!in) { error = relativePath + ": dosya acilamadi"; return {}; }
            std::ostringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }

        std::string Lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return s;
        }

        // "Space", "A", "Left", "F5" -> KeyCode (büyük/küçük harf duyarsız)
        KeyCode KeyFromName(const std::string& name) {
            static const std::unordered_map<std::string, KeyCode> s_Keys = [] {
                std::unordered_map<std::string, KeyCode> m{
                    { "space", Key::Space }, { "enter", Key::Enter }, { "return", Key::Enter },
                    { "escape", Key::Escape }, { "esc", Key::Escape }, { "tab", Key::Tab },
                    { "backspace", Key::Backspace }, { "delete", Key::Delete }, { "insert", Key::Insert },
                    { "left", Key::Left }, { "right", Key::Right }, { "up", Key::Up }, { "down", Key::Down },
                    { "home", Key::Home }, { "end", Key::End }, { "pageup", Key::PageUp }, { "pagedown", Key::PageDown },
                    { "leftshift", Key::LeftShift }, { "rightshift", Key::RightShift }, { "shift", Key::LeftShift },
                    { "leftcontrol", Key::LeftControl }, { "rightcontrol", Key::RightControl }, { "ctrl", Key::LeftControl },
                    { "leftalt", Key::LeftAlt }, { "rightalt", Key::RightAlt }, { "alt", Key::LeftAlt },
                    { "minus", Key::Minus }, { "equal", Key::Equal }, { "comma", Key::Comma }, { "period", Key::Period },
                };
                for (int i = 0; i < 26; ++i) m[std::string(1, (char)('a' + i))] = Key::A + i;
                for (int i = 0; i < 10; ++i) m[std::string(1, (char)('0' + i))] = Key::D0 + i;
                for (int i = 0; i < 12; ++i) m["f" + std::to_string(i + 1)] = Key::F1 + i;
                return m;
            }();
            const auto it = s_Keys.find(Lower(name));
            if (it == s_Keys.end()) throw std::runtime_error("bilinmeyen tus adi: '" + name + "'");
            return it->second;
        }

        MouseButton MouseFromObject(const sol::object& o) {
            if (o.is<int>()) return o.as<int>();
            const std::string n = Lower(o.as<std::string>());
            if (n == "left") return Mouse::Left;
            if (n == "right") return Mouse::Right;
            if (n == "middle") return Mouse::Middle;
            throw std::runtime_error("bilinmeyen fare tusu: '" + n + "'");
        }

        const char* BodyTypeName(BodyType t) {
            switch (t) {
                case BodyType::Static:    return "Static";
                case BodyType::Kinematic: return "Kinematic";
                case BodyType::Dynamic:   return "Dynamic";
            }
            return "Dynamic";
        }

        BodyType BodyTypeFromName(const std::string& n) {
            const std::string l = Lower(n);
            if (l == "static") return BodyType::Static;
            if (l == "kinematic") return BodyType::Kinematic;
            if (l == "dynamic") return BodyType::Dynamic;
            throw std::runtime_error("bilinmeyen govde tipi: '" + n + "' (Static / Kinematic / Dynamic)");
        }

        // Lua değeri ya da nil
        template<typename T>
        sol::object Maybe(sol::this_state s, const EntityRef& r) {
            Entity& e = r.Expect();
            if (!e.GetComponent<T>()) return sol::lua_nil;
            return sol::make_object(s, ComponentRef<T>{ r });
        }

        std::string ArgsToString(sol::variadic_args args, sol::this_state s) {
            sol::state_view lua(s);
            sol::protected_function tostring = lua["tostring"];
            std::string out;
            bool first = true;
            for (auto arg : args) {
                if (!first) out += "\t";
                first = false;
                sol::protected_function_result r = tostring(arg);
                out += r.valid() ? r.get<std::string>() : std::string("?");
            }
            return out;
        }
    }

    // ---------------------------------------------------------------- vec2 / Color
    void RegisterLuaMathTypes(sol::state& lua) {
        lua.new_usertype<glm::vec2>("vec2",
            sol::call_constructor, sol::factories(
                [] { return glm::vec2(0.0f); },
                [](float v) { return glm::vec2(v); },
                [](float x, float y) { return glm::vec2(x, y); }),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            "length", [](const glm::vec2& a) { return glm::length(a); },
            "normalized", [](const glm::vec2& a) { const float l = glm::length(a); return l > 0.0f ? a / l : glm::vec2(0.0f); },
            "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
            "distance", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); },
            "lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return a + (b - a) * t; },
            sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& a, float s) { return a * s; },
                [](float s, const glm::vec2& a) { return a * s; },
                [](const glm::vec2& a, const glm::vec2& b) { return a * b; }),
            sol::meta_function::division, [](const glm::vec2& a, float s) { return a / s; },
            sol::meta_function::unary_minus, [](const glm::vec2& a) { return -a; },
            sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b) { return a == b; },
            sol::meta_function::to_string, [](const glm::vec2& a) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "vec2(%g, %g)", a.x, a.y);
                return std::string(buf);
            });

        lua.new_usertype<glm::vec4>("Color",
            sol::call_constructor, sol::factories(
                [] { return glm::vec4(1.0f); },
                [](float r, float g, float b) { return glm::vec4(r, g, b, 1.0f); },
                [](float r, float g, float b, float a) { return glm::vec4(r, g, b, a); }),
            "r", &glm::vec4::r,
            "g", &glm::vec4::g,
            "b", &glm::vec4::b,
            "a", &glm::vec4::a,
            "lerp", [](const glm::vec4& a, const glm::vec4& b, float t) { return a + (b - a) * t; },
            sol::meta_function::equal_to, [](const glm::vec4& a, const glm::vec4& b) { return a == b; },
            sol::meta_function::to_string, [](const glm::vec4& c) {
                char buf[96];
                std::snprintf(buf, sizeof(buf), "Color(%g, %g, %g, %g)", c.r, c.g, c.b, c.a);
                return std::string(buf);
            });
    }

    // ---------------------------------------------------------------- properties dönüşümü
    sol::object PropertyToLua(sol::state_view lua, const ScriptProperty& p) {
        switch (p.type) {
            case ScriptProperty::Type::Number: return sol::make_object(lua, p.number);
            case ScriptProperty::Type::Bool:   return sol::make_object(lua, p.boolean);
            case ScriptProperty::Type::String: return sol::make_object(lua, p.text);
            case ScriptProperty::Type::Vec2:   return sol::make_object(lua, glm::vec2(p.vec[0], p.vec[1]));
            case ScriptProperty::Type::Color:  return sol::make_object(lua, glm::vec4(p.vec[0], p.vec[1], p.vec[2], p.vec[3]));
        }
        return sol::lua_nil;
    }

    bool PropertyFromLua(const std::string& name, const sol::object& v, ScriptProperty& out) {
        out = ScriptProperty{};
        out.name = name;
        switch (v.get_type()) {
            case sol::type::number:  out.type = ScriptProperty::Type::Number; out.number = v.as<double>(); return true;
            case sol::type::boolean: out.type = ScriptProperty::Type::Bool; out.boolean = v.as<bool>(); return true;
            case sol::type::string:  out.type = ScriptProperty::Type::String; out.text = v.as<std::string>(); return true;
            case sol::type::userdata:
                if (v.is<glm::vec2>()) {
                    const glm::vec2 a = v.as<glm::vec2>();
                    out.type = ScriptProperty::Type::Vec2;
                    out.vec[0] = a.x; out.vec[1] = a.y;
                    return true;
                }
                if (v.is<glm::vec4>()) {
                    const glm::vec4 c = v.as<glm::vec4>();
                    out.type = ScriptProperty::Type::Color;
                    out.vec[0] = c.r; out.vec[1] = c.g; out.vec[2] = c.b; out.vec[3] = c.a;
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    // ---------------------------------------------------------------- ScriptHost
    ScriptHost::ScriptHost(Scene& scene) : m_Scene(scene) {
        m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table,
                             sol::lib::coroutine, sol::lib::utf8);
        Bind();
        m_LastPoll = std::chrono::steady_clock::now();
    }

    ScriptHost::~ScriptHost() = default;

    uint64_t ScriptHost::GetVersion(const std::string& relativePath) {
        auto it = m_Files.find(relativePath);
        if (it == m_Files.end()) {
            std::error_code ec;
            FileState st;
            st.writeTime = fs::last_write_time(Assets::GetPath(relativePath), ec);
            it = m_Files.emplace(relativePath, st).first;
        }
        return it->second.version;
    }

    void ScriptHost::BeginFrame(float dt) {
        m_Time += dt;
        sol::table time = m_Lua["Time"];
        time["dt"] = dt;
        time["time"] = m_Time;

        // Hot-reload: yarım saniyede bir değişen script dosyaları
        const auto now = std::chrono::steady_clock::now();
        if (now - m_LastPoll < std::chrono::milliseconds(500)) return;
        m_LastPoll = now;
        for (auto& [path, st] : m_Files) {
            std::error_code ec;
            const auto t = fs::last_write_time(Assets::GetPath(path), ec);
            if (!ec && t != st.writeTime) {
                st.writeTime = t;
                ++st.version;
                MME_INFO("[Lua] Degisti, yeniden yuklenecek: ", path);
            }
        }
    }

    void ScriptHost::BeginFixedStep(float step) {
        m_Lua["Time"]["fixedStep"] = step;
    }

    std::string ScriptHost::RunFile(const std::string& relativePath, sol::environment& env) {
        std::string error;
        const std::string source = ReadText(relativePath, error);
        if (!error.empty()) return error;
        // "@yol": hata mesajları "scripts/player.lua:12: ..." biçiminde olur
        sol::load_result chunk = m_Lua.load(source, "@" + relativePath);
        if (!chunk.valid()) {
            sol::error e = chunk;
            return e.what();
        }
        sol::protected_function fn = chunk;
        sol::set_environment(env, fn);
        sol::protected_function_result r = fn();
        if (!r.valid()) {
            sol::error e = r;
            return e.what();
        }
        return {};
    }

    bool ScriptHost::ReadDeclaredProperties(const std::string& relativePath,
                                            std::vector<ScriptProperty>& out, std::string& error) {
        out.clear();
        error.clear();
        const std::string source = ReadText(relativePath, error);
        if (!error.empty()) return false;

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
        RegisterLuaMathTypes(lua);
        sol::load_result chunk = lua.load(source, "@" + relativePath);
        if (!chunk.valid()) { sol::error e = chunk; error = e.what(); return false; }
        sol::protected_function fn = chunk;
        sol::protected_function_result r = fn();
        if (!r.valid()) { sol::error e = r; error = e.what(); return false; }

        sol::object props = lua["properties"];
        if (props.get_type() != sol::type::table) return true;
        for (auto& [k, v] : props.as<sol::table>()) {
            if (k.get_type() != sol::type::string) continue;
            ScriptProperty p;
            if (PropertyFromLua(k.as<std::string>(), v, p)) out.push_back(std::move(p));
        }
        // Lua tabloları sırasızdır: inspector'da kararlı görünsün
        std::sort(out.begin(), out.end(), [](const ScriptProperty& a, const ScriptProperty& b) { return a.name < b.name; });
        return true;
    }

    // ---------------------------------------------------------------- bağlamalar
    void ScriptHost::Bind() {
        sol::state& lua = m_Lua;
        Scene* scene = &m_Scene;
        RegisterLuaMathTypes(lua);

        lua.set_function("print", [](sol::variadic_args args, sol::this_state s) {
            MME_INFO("[Lua] ", ArgsToString(args, s));
        });
        sol::table log = lua.create_named_table("Log");
        log.set_function("Info", [](sol::variadic_args args, sol::this_state s) { MME_INFO("[Lua] ", ArgsToString(args, s)); });
        log.set_function("Warn", [](sol::variadic_args args, sol::this_state s) { MME_WARN("[Lua] ", ArgsToString(args, s)); });
        log.set_function("Error", [](sol::variadic_args args, sol::this_state s) { MME_ERROR("[Lua] ", ArgsToString(args, s)); });

        sol::table math = lua["math"];
        math.set_function("clamp", [](double v, double lo, double hi) { return std::clamp(v, lo, hi); });
        math.set_function("lerp", [](double a, double b, double t) { return a + (b - a) * t; });
        math.set_function("sign", [](double v) { return v > 0.0 ? 1.0 : v < 0.0 ? -1.0 : 0.0; });

        lua.create_named_table("Time", "dt", 0.0, "time", 0.0, "fixedStep", 1.0 / 60.0);

        // -------------------------------------------------------- component'ler
        lua.new_usertype<ComponentRef<RigidBody2D>>("RigidBody2D", sol::no_constructor,
            "velocity", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().velocity; },
                [](const ComponentRef<RigidBody2D>& c, const glm::vec2& v) { c.Expect().SetVelocity(v); }),
            "angularVelocity", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().angularVelocity; },
                [](const ComponentRef<RigidBody2D>& c, float v) { c.Expect().angularVelocity = v; }),
            "type", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return std::string(BodyTypeName(c.Expect().type)); },
                [](const ComponentRef<RigidBody2D>& c, const std::string& t) { c.Expect().type = BodyTypeFromName(t); }),
            "gravityScale", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().gravityScale; },
                [](const ComponentRef<RigidBody2D>& c, float v) { c.Expect().gravityScale = v; }),
            "linearDamping", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().linearDamping; },
                [](const ComponentRef<RigidBody2D>& c, float v) { c.Expect().linearDamping = v; }),
            "fixedRotation", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().fixedRotation; },
                [](const ComponentRef<RigidBody2D>& c, bool v) { c.Expect().fixedRotation = v; }),
            "maxSpeed", sol::property(
                [](const ComponentRef<RigidBody2D>& c) { return c.Expect().maxSpeed; },
                [](const ComponentRef<RigidBody2D>& c, float v) { c.Expect().maxSpeed = v; }),
            "mass", sol::readonly_property([](const ComponentRef<RigidBody2D>& c) { return c.Expect().GetMass(); }),
            "AddForce", [](const ComponentRef<RigidBody2D>& c, const glm::vec2& f) { c.Expect().AddForce(f); },
            "AddImpulse", [](const ComponentRef<RigidBody2D>& c, const glm::vec2& dv) { c.Expect().AddImpulse(dv); });

        lua.new_usertype<ComponentRef<SpriteComponent>>("SpriteComponent", sol::no_constructor,
            "tint", sol::property(
                [](const ComponentRef<SpriteComponent>& c) { return c.Expect().tint; },
                [](const ComponentRef<SpriteComponent>& c, const glm::vec4& v) { c.Expect().tint = v; }),
            "size", sol::property(
                [](const ComponentRef<SpriteComponent>& c) { return c.Expect().size; },
                [](const ComponentRef<SpriteComponent>& c, const glm::vec2& v) { c.Expect().size = v; }),
            "SetTexture", [](const ComponentRef<SpriteComponent>& c, const std::string& path) {
                c.Expect().texture = path.empty() ? 0 : Assets::GetTexture(path);
            });

        lua.new_usertype<ComponentRef<SpriteAnimator>>("SpriteAnimator", sol::no_constructor,
            "playing", sol::property(
                [](const ComponentRef<SpriteAnimator>& c) { return c.Expect().playing; },
                [](const ComponentRef<SpriteAnimator>& c, bool v) { c.Expect().playing = v; }),
            "speed", sol::property(
                [](const ComponentRef<SpriteAnimator>& c) { return c.Expect().speed; },
                [](const ComponentRef<SpriteAnimator>& c, float v) { c.Expect().speed = v; }),
            "frame", sol::readonly_property([](const ComponentRef<SpriteAnimator>& c) { return c.Expect().GetFrameInClip(); }),
            // Play(ilkKare, kareSayısı, fps, döngü): aynı klip zaten oynuyorsa baştan almaz
            "Play", [](const ComponentRef<SpriteAnimator>& c, int first, int count, float fps, sol::optional<bool> loop) {
                SpriteAnimator& a = c.Expect();
                const bool l = loop.value_or(true);
                if (a.playing && a.firstFrame == first && a.frameCount == count) { a.fps = fps; a.loop = l; return; }
                a.firstFrame = first;
                a.frameCount = std::max(count, 1);
                a.fps = fps;
                a.loop = l;
                a.playing = true;
                a.SetFrame(0);
            },
            "Stop", [](const ComponentRef<SpriteAnimator>& c) { c.Expect().Stop(); });

        lua.new_usertype<ComponentRef<ParticleEmitter>>("ParticleEmitter", sol::no_constructor,
            "emitting", sol::property(
                [](const ComponentRef<ParticleEmitter>& c) { return c.Expect().emitting; },
                [](const ComponentRef<ParticleEmitter>& c, bool v) { c.Expect().emitting = v; }),
            "rate", sol::property(
                [](const ComponentRef<ParticleEmitter>& c) { return c.Expect().rate; },
                [](const ComponentRef<ParticleEmitter>& c, float v) { c.Expect().rate = v; }),
            "colorBegin", sol::property(
                [](const ComponentRef<ParticleEmitter>& c) { return c.Expect().colorBegin; },
                [](const ComponentRef<ParticleEmitter>& c, const glm::vec4& v) { c.Expect().colorBegin = v; }),
            "colorEnd", sol::property(
                [](const ComponentRef<ParticleEmitter>& c) { return c.Expect().colorEnd; },
                [](const ComponentRef<ParticleEmitter>& c, const glm::vec4& v) { c.Expect().colorEnd = v; }),
            "Burst", [](const ComponentRef<ParticleEmitter>& c, int count) { c.Expect().Burst(count); });

        lua.new_usertype<ComponentRef<CameraComponent>>("CameraComponent", sol::no_constructor,
            "viewHeight", sol::property(
                [](const ComponentRef<CameraComponent>& c) { return c.Expect().viewHeight; },
                [](const ComponentRef<CameraComponent>& c, float v) { c.Expect().viewHeight = v; }),
            "background", sol::property(
                [](const ComponentRef<CameraComponent>& c) { return c.Expect().background; },
                [](const ComponentRef<CameraComponent>& c, const glm::vec4& v) { c.Expect().background = v; }));

        lua.new_usertype<ComponentRef<BoxCollider2D>>("BoxCollider2D", sol::no_constructor,
            "size", sol::property(
                [](const ComponentRef<BoxCollider2D>& c) { return c.Expect().size; },
                [](const ComponentRef<BoxCollider2D>& c, const glm::vec2& v) { c.Expect().size = v; }),
            "offset", sol::property(
                [](const ComponentRef<BoxCollider2D>& c) { return c.Expect().offset; },
                [](const ComponentRef<BoxCollider2D>& c, const glm::vec2& v) { c.Expect().offset = v; }),
            "isTrigger", sol::property(
                [](const ComponentRef<BoxCollider2D>& c) { return c.Expect().isTrigger; },
                [](const ComponentRef<BoxCollider2D>& c, bool v) { c.Expect().isTrigger = v; }));

        lua.new_usertype<ComponentRef<CircleCollider2D>>("CircleCollider2D", sol::no_constructor,
            "radius", sol::property(
                [](const ComponentRef<CircleCollider2D>& c) { return c.Expect().radius; },
                [](const ComponentRef<CircleCollider2D>& c, float v) { c.Expect().radius = v; }),
            "offset", sol::property(
                [](const ComponentRef<CircleCollider2D>& c) { return c.Expect().offset; },
                [](const ComponentRef<CircleCollider2D>& c, const glm::vec2& v) { c.Expect().offset = v; }),
            "isTrigger", sol::property(
                [](const ComponentRef<CircleCollider2D>& c) { return c.Expect().isTrigger; },
                [](const ComponentRef<CircleCollider2D>& c, bool v) { c.Expect().isTrigger = v; }));

        // -------------------------------------------------------- Entity
        lua.new_usertype<EntityRef>("Entity", sol::no_constructor,
            "id", sol::readonly_property([](const EntityRef& r) { return r.id; }),
            "valid", sol::readonly_property([](const EntityRef& r) { return r.Get() != nullptr; }),
            "name", sol::property(
                [](const EntityRef& r) { return r.Expect().GetName(); },
                [](const EntityRef& r, const std::string& n) { r.Expect().SetName(n); }),
            "position", sol::property(
                [](const EntityRef& r) { return r.Expect().position; },
                [](const EntityRef& r, const glm::vec2& v) { r.Expect().position = v; }),
            "x", sol::property(
                [](const EntityRef& r) { return r.Expect().position.x; },
                [](const EntityRef& r, float v) { r.Expect().position.x = v; }),
            "y", sol::property(
                [](const EntityRef& r) { return r.Expect().position.y; },
                [](const EntityRef& r, float v) { r.Expect().position.y = v; }),
            "rotation", sol::property(
                [](const EntityRef& r) { return r.Expect().rotation; },
                [](const EntityRef& r, float v) { r.Expect().rotation = v; }),
            "scale", sol::property(
                [](const EntityRef& r) { return r.Expect().scale; },
                [](const EntityRef& r, const glm::vec2& v) { r.Expect().scale = v; }),
            "zLayer", sol::property(
                [](const EntityRef& r) { return r.Expect().zLayer; },
                [](const EntityRef& r, float v) { r.Expect().zLayer = v; }),
            "active", sol::property(
                [](const EntityRef& r) { return r.Expect().active; },
                [](const EntityRef& r, bool v) { r.Expect().active = v; }),
            "body", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<RigidBody2D>(s, r); }),
            "sprite", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<SpriteComponent>(s, r); }),
            "animator", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<SpriteAnimator>(s, r); }),
            "particles", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<ParticleEmitter>(s, r); }),
            "camera", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<CameraComponent>(s, r); }),
            "box", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<BoxCollider2D>(s, r); }),
            "circle", sol::readonly_property([](const EntityRef& r, sol::this_state s) { return Maybe<CircleCollider2D>(s, r); }),
            // Diğer entity'nin script örneği (self): other.script:TakeDamage(1)
            "script", sol::readonly_property([](const EntityRef& r) -> sol::object {
                auto* sc = r.Expect().GetComponent<ScriptComponent>();
                if (!sc || !sc->GetInstance()) return sol::lua_nil;
                return sc->GetInstance()->self;
            }),
            "Destroy", [](const EntityRef& r) { if (Entity* e = r.Get()) e->Destroy(); },
            "Has", [](const EntityRef& r, const std::string& component) {
                const ComponentInfo* info = ComponentRegistry::Find(component);
                return info && info->get(r.Expect()) != nullptr;
            },
            "Add", [](const EntityRef& r, const std::string& component) {
                const ComponentInfo* info = ComponentRegistry::Find(component);
                if (!info) throw std::runtime_error("bilinmeyen component: '" + component + "'");
                Entity& e = r.Expect();
                if (!info->get(e)) info->add(e);
            },
            "Remove", [](const EntityRef& r, const std::string& component) {
                if (const ComponentInfo* info = ComponentRegistry::Find(component)) info->remove(r.Expect());
            },
            sol::meta_function::equal_to, [](const EntityRef& a, const EntityRef& b) { return a == b; },
            sol::meta_function::to_string, [](const EntityRef& r) {
                const Entity* e = r.Get();
                return "Entity(" + (e ? e->GetName() : std::string("silinmis")) + "#" + std::to_string(r.id) + ")";
            });

        // -------------------------------------------------------- Scene
        sol::table sceneTable = lua.create_named_table("Scene");
        sceneTable.set_function("Find", [scene](const std::string& name) -> sol::optional<EntityRef> {
            if (Entity* e = scene->FindEntity(name)) return EntityRef{ scene, e->GetID() };
            return sol::nullopt;
        });
        sceneTable.set_function("Create", [scene](sol::optional<std::string> name) {
            Entity& e = scene->CreateEntity(name.value_or("Entity"));
            return EntityRef{ scene, e.GetID() };
        });
        sceneTable.set_function("Spawn", [scene](const std::string& prefab, sol::optional<glm::vec2> pos) -> sol::optional<EntityRef> {
            Entity* e = SceneSerializer::Instantiate(*scene, prefab);
            if (!e) return sol::nullopt;
            if (pos) e->position = *pos;
            return EntityRef{ scene, e->GetID() };
        });
        sceneTable.set_function("Load", [scene](const std::string& path) { scene->RequestLoad(path); });
        sceneTable.set_function("Name", [scene] { return scene->GetName(); });

        // -------------------------------------------------------- Input
        sol::table input = lua.create_named_table("Input");
        input.set_function("KeyDown", [](const std::string& k) { return Input::KeyDown(KeyFromName(k)); });
        input.set_function("KeyPressed", [](const std::string& k) { return Input::KeyPressed(KeyFromName(k)); });
        input.set_function("KeyReleased", [](const std::string& k) { return Input::KeyReleased(KeyFromName(k)); });
        input.set_function("MouseDown", [](const sol::object& b) { return Input::MouseDown(MouseFromObject(b)); });
        input.set_function("MousePressed", [](const sol::object& b) { return Input::MousePressed(MouseFromObject(b)); });
        input.set_function("MouseReleased", [](const sol::object& b) { return Input::MouseReleased(MouseFromObject(b)); });
        // -1..1: Input.Axis("A", "D")
        input.set_function("Axis", [](const std::string& negative, const std::string& positive) {
            return (Input::KeyDown(KeyFromName(positive)) ? 1.0f : 0.0f) - (Input::KeyDown(KeyFromName(negative)) ? 1.0f : 0.0f);
        });
        input.set_function("MousePosition", [scene] { return scene->WindowToView(Input::MousePosition()); });
        input.set_function("MouseWorld", [scene] { return scene->WindowToWorld(Input::MousePosition()); });

        // -------------------------------------------------------- Physics
        sol::table physics = lua.create_named_table("Physics");
        physics.set_function("Raycast", [scene](const glm::vec2& from, const glm::vec2& to, sol::optional<EntityRef> ignore,
                                                sol::this_state s) -> sol::object {
            Physics2D::RaycastHit hit;
            const Entity* ignored = ignore ? ignore->Get() : nullptr;
            if (!scene->GetPhysics().Raycast(from, to, hit, ignored)) return sol::lua_nil;
            sol::state_view lv(s);
            sol::table t = lv.create_table();
            t["entity"] = EntityRef{ scene, hit.entity->GetID() };
            t["point"] = hit.point;
            t["normal"] = hit.normal;
            t["fraction"] = hit.fraction;
            return t;
        });
        physics.set_function("SetGravity", [scene](const glm::vec2& g) { scene->GetPhysics().gravity = g; });
        physics.set_function("GetGravity", [scene] { return scene->GetPhysics().gravity; });

        // -------------------------------------------------------- Audio
        sol::table audio = lua.create_named_table("Audio");
        audio.set_function("Play", [](const std::string& path, sol::optional<float> volume, sol::optional<float> pitch) {
            Audio::Play(path, volume.value_or(1.0f), pitch.value_or(1.0f));
        });
        audio.set_function("PlayMusic", [](const std::string& path, sol::optional<bool> loop, sol::optional<float> volume) {
            Audio::PlayMusic(path, loop.value_or(true), volume.value_or(1.0f));
        });
        audio.set_function("StopMusic", [] { Audio::StopMusic(); });

        // -------------------------------------------------------- UI (yalnızca OnUI içinde)
        sol::table ui = lua.create_named_table("UI");
        ui.set_function("Text", [](const std::string& text, float x, float y, sol::optional<float> size, sol::optional<glm::vec4> color) {
            UI::Text(text, { x, y }, size.value_or(24.0f), color.value_or(glm::vec4(1.0f)));
        });
        ui.set_function("TextCentered", [](const std::string& text, float x, float y, sol::optional<float> size, sol::optional<glm::vec4> color) {
            UI::TextCentered(text, { x, y }, size.value_or(24.0f), color.value_or(glm::vec4(1.0f)));
        });
        ui.set_function("Rect", [](float x, float y, float w, float h, sol::optional<glm::vec4> color) {
            Renderer2D::DrawScreenRect({ x, y }, { w, h }, color.value_or(glm::vec4(1.0f)));
        });
        ui.set_function("Measure", [](const std::string& text, sol::optional<float> size) {
            return UI::MeasureText(text, size.value_or(24.0f));
        });
        ui.set_function("Width", [scene] { return scene->GetViewSize().x; });
        ui.set_function("Height", [scene] { return scene->GetViewSize().y; });
    }
}
