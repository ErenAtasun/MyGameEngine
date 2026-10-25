#pragma once
#include <MME/Assets.h>
#include <MME/Entity.h>

#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <type_traits>
#include <typeindex>
#include <vector>

namespace mme {

    // Alanların tipi: inspector bunu nasıl çizeceğini, serileştirici nasıl
    // yazacağını buradan bilir.
    enum class FieldType { Float, Int, Bool, Vec2, Color, String, Enum, Texture };

    struct FieldInfo {
        std::string name;
        FieldType type;
        std::function<void*(Component&)> access;  // component içindeki üyenin adresi
        std::vector<std::string> enumNames;       // yalnızca Enum
        float dragSpeed = 1.0f;                   // inspector sürükleme hızı
        bool hidden = false;                      // serileştirilir ama inspector'da gösterilmez
    };

    struct ComponentInfo {
        std::string name;
        std::type_index type;
        std::vector<FieldInfo> fields;
        std::function<Component&(Entity&)> add;
        std::function<Component*(Entity&)> get;
        std::function<void(Entity&)> remove;
        // İsteğe bağlı: alanlardan sonra çizilen özel inspector (ör. script properties).
        // beforeEdit değer değişmeden hemen önce çağrılmalı. Değiştiyse true döner.
        std::function<bool(Component&, const std::function<void()>& beforeEdit)> inspector;
    };

    template<typename T> class ComponentBuilder;

    // Component tiplerinin tek kaydı: hem debug inspector hem sahne serileştirme
    // buradan beslenir. Oyun kendi component'lerini bir kez kaydeder:
    //
    //   ComponentRegistry::Register<PaddleController>("PaddleController")
    //       .Field("moveSpeed", &PaddleController::moveSpeed)
    //       .Field("minY", &PaddleController::minY);
    class ComponentRegistry {
    public:
        template<typename T>
        static ComponentBuilder<T> Register(const std::string& name) {
            static_assert(std::is_base_of_v<Component, T>, "T, mme::Component'ten turemeli");
            static_assert(std::is_default_constructible_v<T>, "Kayitli component varsayilan kurucuya sahip olmali");
            ComponentInfo info{
                name, std::type_index(typeid(T)), {},
                [](Entity& e) -> Component& { return e.AddComponent<T>(); },
                [](Entity& e) -> Component* { return e.GetComponent<T>(); },
                [](Entity& e) { e.RemoveComponent<T>(); },
            };
            return ComponentBuilder<T>(Add(std::move(info)));
        }

        static const ComponentInfo* Find(std::type_index type);
        static const ComponentInfo* Find(const std::string& name);
        static const std::vector<ComponentInfo>& GetAll();

        // Motorun kendi component'leri (Application çağırır; tekrar çağrılırsa bir şey yapmaz)
        static void RegisterBuiltins();

    private:
        template<typename T> friend class ComponentBuilder;
        static size_t Add(ComponentInfo info);   // aynı isim varsa yenisiyle değiştirir
        static ComponentInfo& At(size_t index);
    };

    template<typename T>
    class ComponentBuilder {
    public:
        explicit ComponentBuilder(size_t index) : m_Index(index) {}

        // float, int, bool, glm::vec2, glm::vec4 (renk), std::string.
        // Üye T'nin bir taban sınıfında da olabilir (ör. Collider2D::friction).
        template<typename M, typename C>
        ComponentBuilder& Field(const std::string& name, M C::* member, float dragSpeed = 1.0f) {
            static_assert(std::is_base_of_v<C, T>, "Alan bu component'in (ya da tabanının) uyesi olmali");
            FieldType type;
            if constexpr (std::is_same_v<M, float>)            type = FieldType::Float;
            else if constexpr (std::is_same_v<M, bool>)        type = FieldType::Bool;
            else if constexpr (std::is_same_v<M, int>)         type = FieldType::Int;
            else if constexpr (std::is_same_v<M, glm::vec2>)   type = FieldType::Vec2;
            else if constexpr (std::is_same_v<M, glm::vec4>)   type = FieldType::Color;
            else if constexpr (std::is_same_v<M, std::string>) type = FieldType::String;
            else static_assert(sizeof(M) == 0, "Desteklenmeyen alan tipi: Enum() ya da Texture() kullanin");
            AddField(name, type, member).dragSpeed = dragSpeed;
            return *this;
        }

        // int tabanlı enum'lar: isimler sırayla 0, 1, 2... değerlerine karşılık gelir
        template<typename E, typename C>
        ComponentBuilder& Enum(const std::string& name, E C::* member, std::vector<std::string> names) {
            static_assert(std::is_base_of_v<C, T>, "Alan bu component'in (ya da tabanının) uyesi olmali");
            static_assert(std::is_enum_v<E> && sizeof(E) == sizeof(int), "int boyutunda enum olmali");
            AddField(name, FieldType::Enum, member).enumNames = std::move(names);
            return *this;
        }

        // Son eklenen alanı inspector'dan gizler (yine de serileştirilir)
        ComponentBuilder& Hidden() {
            auto& fields = ComponentRegistry::At(m_Index).fields;
            if (!fields.empty()) fields.back().hidden = true;
            return *this;
        }

        ComponentBuilder& Inspector(std::function<bool(Component&, const std::function<void()>&)> fn) {
            ComponentRegistry::At(m_Index).inspector = std::move(fn);
            return *this;
        }

        // Doku: dosyaya asset yolu olarak yazılır
        template<typename C>
        ComponentBuilder& Texture(const std::string& name, TextureID C::* member) {
            static_assert(std::is_base_of_v<C, T>, "Alan bu component'in (ya da tabanının) uyesi olmali");
            AddField(name, FieldType::Texture, member);
            return *this;
        }

    private:
        template<typename M, typename C>
        FieldInfo& AddField(const std::string& name, FieldType type, M C::* member) {
            FieldInfo f;
            f.name = name;
            f.type = type;
            f.access = [member](Component& c) -> void* { return &(static_cast<C&>(static_cast<T&>(c)).*member); };
            auto& fields = ComponentRegistry::At(m_Index).fields;
            fields.push_back(std::move(f));
            return fields.back();
        }

        size_t m_Index;
    };
}
