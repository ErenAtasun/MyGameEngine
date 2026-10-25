#pragma once
#include <string>

namespace mme {

    class Scene;

    // Sahneyi JSON olarak kaydeder/yükler. Yalnızca ComponentRegistry'de kayıtlı
    // component'ler ve onların kayıtlı alanları yazılır; callback'ler (std::function)
    // ve isimli animasyon klipleri gibi koddan kurulan şeyler yüklemeden sonra
    // oyun tarafından yeniden bağlanmalıdır.
    //
    // Load sahneyi önce temizler: sahnedeki Entity* işaretçileri geçersiz olur.
    namespace SceneSerializer {
        bool Save(Scene& scene, const std::string& relativePath);   // assets klasörüne göre
        bool Load(Scene& scene, const std::string& relativePath);

        std::string ToJson(Scene& scene);
        bool FromJson(Scene& scene, const std::string& json);
    }
}
