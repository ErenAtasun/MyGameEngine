// ScriptComponent'in inspector'ı: script'in properties tablosundaki değerler.
// Düzenleme modunda değerler ScriptComponent::properties'e (JSON) yazılır;
// script çalışırken (Play) doğrudan self tablosundaki canlı değerler düzenlenir.
#include "ScriptHost.h"
#include <MME/Assets.h>
#include <MME/Component.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <unordered_map>

namespace fs = std::filesystem;

namespace mme {

    namespace {
        struct CachedScript {
            fs::file_time_type writeTime{};
            std::vector<ScriptProperty> props;
            std::string error;
        };

        // Dosya değişmedikçe script yeniden çalıştırılmaz
        const CachedScript& Declared(const std::string& path) {
            static std::unordered_map<std::string, CachedScript> s_Cache;
            std::error_code ec;
            const auto t = fs::last_write_time(Assets::GetPath(path), ec);
            auto it = s_Cache.find(path);
            if (it == s_Cache.end() || it->second.writeTime != t) {
                CachedScript c;
                c.writeTime = t;
                if (ec) c.error = "dosya bulunamadi";
                else ScriptHost::ReadDeclaredProperties(path, c.props, c.error);
                it = s_Cache.insert_or_assign(path, std::move(c)).first;
            }
            return it->second;
        }

        nlohmann::json ToJson(const ScriptProperty& p) {
            switch (p.type) {
                case ScriptProperty::Type::Number: return p.number;
                case ScriptProperty::Type::Bool:   return p.boolean;
                case ScriptProperty::Type::String: return p.text;
                case ScriptProperty::Type::Vec2:   return nlohmann::json::array({ p.vec[0], p.vec[1] });
                case ScriptProperty::Type::Color:  return nlohmann::json::array({ p.vec[0], p.vec[1], p.vec[2], p.vec[3] });
            }
            return nullptr;
        }

        // Kayıtlı değeri (tipi uyuyorsa) varsayılanın üstüne yazar
        void FromJson(const nlohmann::json& j, ScriptProperty& p) {
            switch (p.type) {
                case ScriptProperty::Type::Number: if (j.is_number()) p.number = j.get<double>(); break;
                case ScriptProperty::Type::Bool:   if (j.is_boolean()) p.boolean = j.get<bool>(); break;
                case ScriptProperty::Type::String: if (j.is_string()) p.text = j.get<std::string>(); break;
                case ScriptProperty::Type::Vec2:
                    if (j.is_array() && j.size() == 2) for (size_t i = 0; i < 2; ++i) p.vec[i] = j[i].get<float>();
                    break;
                case ScriptProperty::Type::Color:
                    if (j.is_array() && j.size() == 4) for (size_t i = 0; i < 4; ++i) p.vec[i] = j[i].get<float>();
                    break;
            }
        }

        // Tek değerin widget'ı; activated: düzenleme bu frame başladı
        bool DrawValue(ScriptProperty& p, bool& activated) {
            bool changed = false;
            const char* label = p.name.c_str();
            switch (p.type) {
                case ScriptProperty::Type::Number: {
                    float v = (float)p.number;
                    if (ImGui::DragFloat(label, &v, 0.5f)) { p.number = v; changed = true; }
                    break;
                }
                case ScriptProperty::Type::Bool:   changed = ImGui::Checkbox(label, &p.boolean); break;
                case ScriptProperty::Type::String: changed = ImGui::InputText(label, &p.text); break;
                case ScriptProperty::Type::Vec2:   changed = ImGui::DragFloat2(label, p.vec, 0.5f); break;
                case ScriptProperty::Type::Color:  changed = ImGui::ColorEdit4(label, p.vec); break;
            }
            activated = ImGui::IsItemActivated();
            return changed;
        }

        bool DrawLive(ScriptComponent::Instance& inst, const std::vector<ScriptProperty>& declared) {
            bool changed = false;
            ImGui::TextDisabled("Canlı değerler (oyun çalışıyor)");
            sol::state_view lua(inst.self.lua_state());
            for (const ScriptProperty& d : declared) {
                ScriptProperty p;
                if (!PropertyFromLua(d.name, inst.self[d.name], p)) continue;
                bool activated = false;
                if (DrawValue(p, activated)) {
                    inst.self[d.name] = PropertyToLua(lua, p);
                    changed = true;
                }
            }
            return changed;
        }
    }

    bool DrawScriptProperties(Component& component, const std::function<void()>& beforeEdit) {
        auto& sc = static_cast<ScriptComponent&>(component);
        if (!sc.GetError().empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
            ImGui::TextWrapped("%s", sc.GetError().c_str());
            ImGui::PopStyleColor();
        }
        if (sc.script.empty()) {
            ImGui::TextDisabled("Bir .lua dosyasını script alanına sürükleyin");
            return false;
        }

        const CachedScript& cached = Declared(sc.script);
        if (!cached.error.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", cached.error.c_str());
            ImGui::PopStyleColor();
            return false;
        }
        if (cached.props.empty()) return false;

        if (sc.IsRunning() && sc.GetInstance() && sc.GetInstance()->self.valid())
            return DrawLive(*sc.GetInstance(), cached.props);

        nlohmann::json values = sc.properties.empty() ? nlohmann::json::object()
                                                      : nlohmann::json::parse(sc.properties, nullptr, false);
        if (!values.is_object()) values = nlohmann::json::object();

        bool changed = false;
        for (ScriptProperty p : cached.props) {
            const auto it = values.find(p.name);
            const bool overridden = it != values.end();
            if (overridden) FromJson(*it, p);

            ImGui::PushID(p.name.c_str());
            // Varsayılandan farklı değerler vurgulanır; sağ tık menüsünden sıfırlanır
            if (overridden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.85f, 1.0f, 1.0f));
            bool activated = false;
            const bool edited = DrawValue(p, activated);
            if (overridden) ImGui::PopStyleColor();
            // Checkbox/Color tıklamada hemen değişir: anlık görüntü değişiklikten önce alınmalı
            if (activated || (edited && !ImGui::IsItemActive())) beforeEdit();
            if (edited) {
                values[p.name] = ToJson(p);
                changed = true;
            }
            if (overridden && ImGui::BeginPopupContextItem("reset")) {
                if (ImGui::MenuItem("Varsayılana döndür")) {
                    beforeEdit();
                    values.erase(p.name);
                    changed = true;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
        if (changed) sc.properties = values.empty() ? std::string() : values.dump();
        return changed;
    }
}
