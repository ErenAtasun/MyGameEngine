#include <MME/InspectorUI.h>
#include <MME/Assets.h>
#include <MME/ComponentRegistry.h>
#include <MME/Entity.h>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <typeindex>

namespace mme::InspectorUI {

    namespace {
        void Before(const Hooks& hooks) {
            if (hooks.beforeEdit) hooks.beforeEdit();
        }

        // Sürükle/yazmaya başlanan widget'ın değeri henüz değişmedi: anlık görüntü zamanı
        bool Track(bool changed, const Hooks& hooks) {
            if (ImGui::IsItemActivated()) Before(hooks);
            return changed;
        }

        bool IsImagePath(const std::string& path) {
            std::string ext = path.substr(path.find_last_of('.') + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga";
        }

        // Asset tarayıcıdan bırakılan dosyanın yolu; bırakılmadıysa boş
        std::string AcceptAssetDrop() {
            std::string path;
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(kAssetPayload))
                    path.assign(static_cast<const char*>(p->Data));
                ImGui::EndDragDropTarget();
            }
            return path;
        }
    }

    bool DrawField(const FieldInfo& f, Component& c, const Hooks& hooks) {
        void* p = f.access(c);
        const char* label = f.name.c_str();
        switch (f.type) {
            case FieldType::Float:  return Track(ImGui::DragFloat(label, static_cast<float*>(p), f.dragSpeed), hooks);
            case FieldType::Int:    return Track(ImGui::DragInt(label, static_cast<int*>(p), std::max(f.dragSpeed, 0.1f)), hooks);
            case FieldType::Bool:   return Track(ImGui::Checkbox(label, static_cast<bool*>(p)), hooks);
            case FieldType::Vec2:   return Track(ImGui::DragFloat2(label, &static_cast<glm::vec2*>(p)->x, f.dragSpeed), hooks);
            case FieldType::Color:  return Track(ImGui::ColorEdit4(label, &static_cast<glm::vec4*>(p)->x), hooks);
            case FieldType::String: {
                auto* str = static_cast<std::string*>(p);
                bool changed = Track(ImGui::InputText(label, str), hooks);
                const std::string dropped = AcceptAssetDrop();
                if (!dropped.empty()) { Before(hooks); *str = dropped; changed = true; }
                return changed;
            }
            case FieldType::Enum: {
                int* v = static_cast<int*>(p);
                const char* preview = (*v >= 0 && *v < (int)f.enumNames.size()) ? f.enumNames[(size_t)*v].c_str() : "?";
                bool changed = false;
                if (ImGui::BeginCombo(label, preview)) {
                    for (int i = 0; i < (int)f.enumNames.size(); ++i)
                        if (ImGui::Selectable(f.enumNames[(size_t)i].c_str(), i == *v) && i != *v) {
                            Before(hooks);
                            *v = i;
                            changed = true;
                        }
                    ImGui::EndCombo();
                }
                return changed;
            }
            case FieldType::Texture: {
                TextureID& tex = *static_cast<TextureID*>(p);
                const std::string path = Assets::GetTexturePath(tex);
                bool changed = false;

                ImGui::PushID(label);
                const float w = 64.0f;
                if (tex) {
                    const TextureInfo info = Assets::GetTextureInfo(tex);
                    const float h = info.width > 0 ? std::min(w * (float)info.height / (float)info.width, 96.0f) : w;
                    // Dokularda v=0 altta; ImGui y'si aşağı baktığı için v çevrilir
                    ImGui::Image((ImTextureID)(uintptr_t)tex, ImVec2(w, h), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
                } else {
                    ImGui::Button("(yok)", ImVec2(w, w));
                }
                std::string dropped = AcceptAssetDrop();
                if (!dropped.empty() && IsImagePath(dropped)) {
                    Before(hooks);
                    tex = Assets::GetTexture(dropped);
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted(label);
                ImGui::TextDisabled("%s", tex == 0 ? "sürükleyip bırakın" : path.empty() ? "(bellekten)" : path.c_str());
                if (tex) {
                    bool nearest = Assets::GetTextureInfo(tex).filter == TextureFilter::Nearest;
                    if (ImGui::Checkbox("Piksel", &nearest)) {
                        Assets::SetTextureFilter(tex, nearest ? TextureFilter::Nearest : TextureFilter::Linear);
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Kaldır")) { Before(hooks); tex = 0; changed = true; }
                }
                ImGui::EndGroup();
                ImGui::PopID();
                return changed;
            }
        }
        return false;
    }

    bool DrawComponent(const ComponentInfo& info, Component& c, const Hooks& hooks) {
        bool changed = false;
        for (const FieldInfo& f : info.fields)
            if (!f.hidden) changed |= DrawField(f, c, hooks);
        if (info.inspector) {
            const std::function<void()> before = [&hooks] { Before(hooks); };
            changed |= info.inspector(c, before);
        }
        return changed;
    }

    bool DrawEntity(Entity& e, const Hooks& hooks) {
        bool changed = false;
        ImGui::PushID((int)e.GetID());

        std::string name = e.GetName();
        if (Track(ImGui::InputText("İsim", &name), hooks)) { e.SetName(name); changed = true; }
        changed |= Track(ImGui::Checkbox("Aktif", &e.active), hooks);
        ImGui::SameLine();
        ImGui::TextDisabled("ID %u", e.GetID());
        changed |= Track(ImGui::DragFloat2("Konum", &e.position.x, 1.0f), hooks);
        changed |= Track(ImGui::DragFloat("Rotasyon", &e.rotation, 1.0f), hooks);
        changed |= Track(ImGui::DragFloat2("Ölçek", &e.scale.x, 0.01f), hooks);
        changed |= Track(ImGui::DragFloat("zLayer", &e.zLayer, 0.1f), hooks);

        std::type_index toRemove = typeid(void);
        e.ForEachComponent([&](std::type_index type, Component& c) {
            const ComponentInfo* info = ComponentRegistry::Find(type);
            ImGui::PushID(type.name());
            if (!info) {
                ImGui::Separator();
                ImGui::TextDisabled("%s (kayıtsız)", type.name());
                ImGui::PopID();
                return;
            }
            const bool open = ImGui::CollapsingHeader(info->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10.0f);
            if (ImGui::SmallButton("X")) toRemove = type;
            if (open) changed |= DrawComponent(*info, c, hooks);
            ImGui::PopID();
        });
        // Dolaşım bittikten sonra sil: döngü içinde vektör değişmesin
        if (toRemove != std::type_index(typeid(void))) {
            Before(hooks);
            e.RemoveComponent(toRemove);
            changed = true;
        }

        ImGui::Spacing();
        if (ImGui::BeginCombo("##add", "Component ekle...")) {
            for (const ComponentInfo& info : ComponentRegistry::GetAll()) {
                if (info.get(e)) continue;
                if (ImGui::Selectable(info.name.c_str())) {
                    Before(hooks);
                    info.add(e);
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        return changed;
    }
}
