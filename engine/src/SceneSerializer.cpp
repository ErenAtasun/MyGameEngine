#include <MME/SceneSerializer.h>
#include <MME/Assets.h>
#include <MME/ComponentRegistry.h>
#include <MME/Log.h>
#include <MME/Scene.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace mme::SceneSerializer {

    // ordered_json: component'ler eklenme sırasıyla yazılır. Sıra önemli: çizim ve
    // güncelleme sırası buna bağlı (ör. parçacık izi sprite'ın arkasında kalmalı).
    using json = nlohmann::ordered_json;

    namespace {
        constexpr int kFormatVersion = 1;

        json Vec2(const glm::vec2& v) { return json::array({ v.x, v.y }); }
        json Vec4(const glm::vec4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

        glm::vec2 ReadVec2(const json& j, glm::vec2 fallback) {
            if (!j.is_array() || j.size() != 2) return fallback;
            return { j[0].get<float>(), j[1].get<float>() };
        }
        glm::vec4 ReadVec4(const json& j, glm::vec4 fallback) {
            if (!j.is_array() || j.size() != 4) return fallback;
            return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>() };
        }

        json WriteField(const FieldInfo& f, void* p) {
            switch (f.type) {
                case FieldType::Float:  return *static_cast<float*>(p);
                case FieldType::Int:    return *static_cast<int*>(p);
                case FieldType::Bool:   return *static_cast<bool*>(p);
                case FieldType::Vec2:   return Vec2(*static_cast<glm::vec2*>(p));
                case FieldType::Color:  return Vec4(*static_cast<glm::vec4*>(p));
                case FieldType::String: return *static_cast<std::string*>(p);
                case FieldType::Enum: {
                    const int v = *static_cast<int*>(p);
                    return (v >= 0 && v < (int)f.enumNames.size()) ? json(f.enumNames[(size_t)v]) : json(v);
                }
                case FieldType::Texture: {
                    const TextureID tex = *static_cast<TextureID*>(p);
                    if (!tex) return nullptr;
                    const std::string path = Assets::GetTexturePath(tex);
                    if (path.empty()) return nullptr; // bellekten oluşturulmuş doku kaydedilemez
                    const bool nearest = Assets::GetTextureInfo(tex).filter == TextureFilter::Nearest;
                    return json{ { "path", path }, { "filter", nearest ? "nearest" : "linear" } };
                }
            }
            return nullptr;
        }

        void ReadField(const FieldInfo& f, void* p, const json& j) {
            switch (f.type) {
                case FieldType::Float:  if (j.is_number()) *static_cast<float*>(p) = j.get<float>(); break;
                case FieldType::Int:    if (j.is_number()) *static_cast<int*>(p) = j.get<int>(); break;
                case FieldType::Bool:   if (j.is_boolean()) *static_cast<bool*>(p) = j.get<bool>(); break;
                case FieldType::Vec2:   *static_cast<glm::vec2*>(p) = ReadVec2(j, *static_cast<glm::vec2*>(p)); break;
                case FieldType::Color:  *static_cast<glm::vec4*>(p) = ReadVec4(j, *static_cast<glm::vec4*>(p)); break;
                case FieldType::String: if (j.is_string()) *static_cast<std::string*>(p) = j.get<std::string>(); break;
                case FieldType::Enum: {
                    if (j.is_string()) {
                        for (size_t i = 0; i < f.enumNames.size(); ++i)
                            if (f.enumNames[i] == j.get<std::string>()) { *static_cast<int*>(p) = (int)i; break; }
                    } else if (j.is_number()) {
                        *static_cast<int*>(p) = j.get<int>();
                    }
                    break;
                }
                case FieldType::Texture: {
                    TextureID& tex = *static_cast<TextureID*>(p);
                    if (j.is_null()) { tex = 0; break; }
                    if (!j.is_object() || !j.contains("path")) break;
                    const bool nearest = j.value("filter", std::string("linear")) == "nearest";
                    const TextureFilter filter = nearest ? TextureFilter::Nearest : TextureFilter::Linear;
                    tex = Assets::GetTexture(j["path"].get<std::string>(), filter);
                    Assets::SetTextureFilter(tex, filter); // önbellekte başka filtreyle yüklenmiş olabilir
                    break;
                }
            }
        }
    }

    namespace {
        json WriteEntity(Entity& e, std::unordered_set<std::string>& skipped) {
            json je;
            je["id"] = e.GetID();
            je["name"] = e.GetName();
            je["active"] = e.active;
            je["transform"] = {
                { "position", Vec2(e.position) },
                { "rotation", e.rotation },
                { "scale", Vec2(e.scale) },
                { "zLayer", e.zLayer },
            };
            json& comps = je["components"] = json::object();
            e.ForEachComponent([&](std::type_index type, Component& c) {
                const ComponentInfo* info = ComponentRegistry::Find(type);
                if (!info) { skipped.insert(type.name()); return; }
                json jc = json::object();
                for (const FieldInfo& f : info->fields)
                    jc[f.name] = WriteField(f, f.access(c));
                comps[info->name] = std::move(jc);
            });
            return je;
        }

        // keepID = false: yeni ID verilir (prefab, kopya)
        Entity& ReadEntity(Scene& scene, const json& je, bool keepID) {
            const std::string name = je.value("name", std::string("Entity"));
            Entity& e = keepID ? scene.CreateEntityWithID(je.value("id", 0u), name) : scene.CreateEntity(name);
            e.active = je.value("active", true);
            if (const auto t = je.find("transform"); t != je.end()) {
                e.position = ReadVec2(t->value("position", json()), e.position);
                e.rotation = t->value("rotation", e.rotation);
                e.scale = ReadVec2(t->value("scale", json()), e.scale);
                e.zLayer = t->value("zLayer", e.zLayer);
            }
            // value() geçici nesne döner: items() onu referansla dolaştığı için önce sakla
            const json components = je.value("components", json::object());
            for (const auto& [compName, jc] : components.items()) {
                const ComponentInfo* info = ComponentRegistry::Find(compName);
                if (!info) {
                    MME_WARN("[SceneSerializer] Bilinmeyen component '", compName, "' (", e.GetName(), ")");
                    continue;
                }
                Component& c = info->get(e) ? *info->get(e) : info->add(e);
                for (const FieldInfo& f : info->fields)
                    if (const auto it = jc.find(f.name); it != jc.end())
                        ReadField(f, f.access(c), *it);
            }
            return e;
        }

        void WarnSkipped(const std::unordered_set<std::string>& skipped) {
            for (const auto& name : skipped)
                MME_WARN("[SceneSerializer] Kayitsiz component atlandi: ", name);
        }
    }

    std::string ToJson(Scene& scene) {
        json root;
        root["format"] = kFormatVersion;
        root["scene"] = scene.GetName();
        root["physics"] = {
            { "gravity", Vec2(scene.GetPhysics().gravity) },
            { "subSteps", scene.GetPhysics().subSteps },
        };
        json& entities = root["entities"] = json::array();

        std::unordered_set<std::string> skipped;
        for (auto& owned : scene.GetEntities())
            if (!owned->IsPendingDestroy()) entities.push_back(WriteEntity(*owned, skipped));
        WarnSkipped(skipped);
        return root.dump(2);
    }

    bool FromJson(Scene& scene, const std::string& text) {
        json root = json::parse(text, nullptr, false);
        if (root.is_discarded() || !root.is_object()) {
            MME_ERROR("[SceneSerializer] Gecersiz JSON");
            return false;
        }
        if (root.value("format", 0) != kFormatVersion) {
            MME_ERROR("[SceneSerializer] Desteklenmeyen format surumu: ", root.value("format", 0));
            return false;
        }

        scene.Clear();
        if (root.contains("scene") && root["scene"].is_string()) scene.SetName(root["scene"].get<std::string>());
        if (const auto p = root.find("physics"); p != root.end() && p->is_object()) {
            Physics2D& physics = scene.GetPhysics();
            physics.gravity = ReadVec2(p->value("gravity", json()), physics.gravity);
            physics.subSteps = p->value("subSteps", physics.subSteps);
        }
        for (const json& je : root.value("entities", json::array()))
            ReadEntity(scene, je, true);
        return true;
    }

    std::string EntityToJson(Entity& entity) {
        std::unordered_set<std::string> skipped;
        json root;
        root["format"] = kFormatVersion;
        root["entity"] = WriteEntity(entity, skipped);
        root["entity"].erase("id");
        WarnSkipped(skipped);
        return root.dump(2);
    }

    Entity* EntityFromJson(Scene& scene, const std::string& text) {
        json root = json::parse(text, nullptr, false);
        if (root.is_discarded() || !root.is_object() || !root.contains("entity")) {
            MME_ERROR("[SceneSerializer] Gecersiz prefab JSON");
            return nullptr;
        }
        return &ReadEntity(scene, root["entity"], false);
    }

    bool SavePrefab(Entity& entity, const std::string& relativePath) {
        const std::filesystem::path path = Assets::GetPath(relativePath);
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            MME_ERROR("[SceneSerializer] Yazilamadi: ", path.string());
            return false;
        }
        out << EntityToJson(entity);
        MME_INFO("[SceneSerializer] Prefab kaydedildi: ", relativePath);
        return true;
    }

    Entity* Instantiate(Scene& scene, const std::string& relativePath) {
        std::ifstream in(Assets::GetPath(relativePath), std::ios::binary);
        if (!in) {
            MME_ERROR("[SceneSerializer] Prefab acilamadi: ", relativePath);
            return nullptr;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        return EntityFromJson(scene, ss.str());
    }

    bool Save(Scene& scene, const std::string& relativePath) {
        const std::filesystem::path path = Assets::GetPath(relativePath);
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream out(path, std::ios::binary);
        if (!out) {
            MME_ERROR("[SceneSerializer] Yazilamadi: ", path.string());
            return false;
        }
        out << ToJson(scene);
        MME_INFO("[SceneSerializer] Kaydedildi: ", relativePath, " (", scene.GetEntityCount(), " entity)");
        return true;
    }

    bool Load(Scene& scene, const std::string& relativePath) {
        std::ifstream in(Assets::GetPath(relativePath), std::ios::binary);
        if (!in) {
            MME_ERROR("[SceneSerializer] Acilamadi: ", relativePath);
            return false;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        if (!FromJson(scene, ss.str())) return false;
        MME_INFO("[SceneSerializer] Yuklendi: ", relativePath, " (", scene.GetEntityCount(), " entity)");
        return true;
    }
}
