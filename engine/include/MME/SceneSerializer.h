#pragma once
#include <string>

namespace mme {

    class Entity;
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

        // Prefab: tek entity'nin kopyası. Oluşturulan entity yeni bir ID alır.
        std::string EntityToJson(Entity& entity);
        Entity* EntityFromJson(Scene& scene, const std::string& json);
        bool SavePrefab(Entity& entity, const std::string& relativePath);
        Entity* Instantiate(Scene& scene, const std::string& relativePath);
    }
}
