#include <MME/Project.h>
#include <MME/Log.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

namespace mme {

    bool Project::Load(const std::string& directory, Project& out) {
        std::ifstream in(fs::path(directory) / kFileName, std::ios::binary);
        if (!in) {
            MME_ERROR("[Project] Bulunamadi: ", (fs::path(directory) / kFileName).string());
            return false;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        const json j = json::parse(ss.str(), nullptr, false);
        if (j.is_discarded() || !j.is_object()) {
            MME_ERROR("[Project] Gecersiz project.json: ", directory);
            return false;
        }
        Project p;
        p.name = j.value("name", p.name);
        p.startScene = j.value("startScene", p.startScene);
        p.escapeQuits = j.value("escapeQuits", p.escapeQuits);
        if (const auto w = j.find("window"); w != j.end() && w->is_object()) {
            p.windowWidth = w->value("width", p.windowWidth);
            p.windowHeight = w->value("height", p.windowHeight);
        }
        p.directory = directory;
        out = std::move(p);
        return true;
    }

    bool Project::Save() const {
        json j;
        j["name"] = name;
        j["startScene"] = startScene;
        j["window"] = { { "width", windowWidth }, { "height", windowHeight } };
        j["escapeQuits"] = escapeQuits;
        std::ofstream out(fs::path(directory) / kFileName, std::ios::binary);
        if (!out) {
            MME_ERROR("[Project] Yazilamadi: ", directory);
            return false;
        }
        out << j.dump(2);
        return true;
    }

    bool Project::Create(const std::string& directory, const std::string& name, Project& out) {
        std::error_code ec;
        if (fs::exists(fs::path(directory) / kFileName, ec)) {
            MME_ERROR("[Project] Zaten bir proje var: ", directory);
            return false;
        }
        for (const char* sub : { "scenes", "scripts", "sprites", "prefabs", "sounds" })
            fs::create_directories(fs::path(directory) / sub, ec);
        if (ec) {
            MME_ERROR("[Project] Klasor olusturulamadi: ", directory, " (", ec.message(), ")");
            return false;
        }

        Project p;
        p.name = name;
        p.directory = directory;
        if (!p.Save()) return false;

        // Kameralı boş sahne
        const json scene = {
            { "format", 1 },
            { "scene", "main" },
            { "physics", { { "gravity", { 0.0, -1500.0 } }, { "subSteps", 4 } } },
            { "entities", json::array({ {
                { "id", 1 }, { "name", "Kamera" }, { "active", true },
                { "transform", { { "position", { 0.0, 0.0 } }, { "rotation", 0.0 }, { "scale", { 1.0, 1.0 } }, { "zLayer", 0.0 } } },
                { "components", { { "CameraComponent", { { "viewHeight", 720.0 }, { "primary", true },
                                                         { "background", { 0.10, 0.11, 0.15, 1.0 } } } } } },
            } }) },
        };
        std::ofstream(fs::path(directory) / p.startScene, std::ios::binary) << scene.dump(2);
        MME_INFO("[Project] Olusturuldu: ", fs::absolute(directory).string());
        out = std::move(p);
        return true;
    }
}
