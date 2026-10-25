#pragma once
#include <string>

namespace mme {

    // Editörde oluşturulan bir oyun projesi: klasördeki project.json.
    // Proje klasörü asset köküdür (sahneler, script'ler, görseller yolları buna göre).
    //
    //   MyGame/
    //     project.json      { "name": ..., "startScene": "scenes/main.json", "window": {...} }
    //     scenes/  scripts/  sprites/  prefabs/  sounds/
    struct Project {
        std::string name = "Yeni Oyun";
        std::string startScene = "scenes/main.json";
        int windowWidth = 1280;
        int windowHeight = 720;
        bool escapeQuits = true;     // oynatıcıda Esc oyunu kapatır

        std::string directory;       // project.json'un bulunduğu klasör (yüklenince doldurulur)

        static constexpr const char* kFileName = "project.json";

        static bool Load(const std::string& directory, Project& out);
        bool Save() const;
        // Klasörü, alt klasörleri, project.json'u ve kameralı boş bir başlangıç sahnesi oluşturur
        static bool Create(const std::string& directory, const std::string& name, Project& out);
    };
}
