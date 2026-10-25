#include "EditorLayer.h"

#include <imgui.h>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <typeindex>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
    const glm::vec4 kAxisX{ 0.95f, 0.30f, 0.30f, 1.0f };
    const glm::vec4 kAxisY{ 0.40f, 0.90f, 0.40f, 1.0f };
    const glm::vec4 kFree{ 0.35f, 0.65f, 1.00f, 1.0f };
    const glm::vec4 kHot{ 1.00f, 0.90f, 0.25f, 1.0f };
    const glm::vec4 kSelection{ 1.00f, 0.60f, 0.15f, 1.0f };
    constexpr float kGizmoLengthPx = 70.0f;
    constexpr float kHandlePx = 7.0f;

    glm::vec2 Rotate(const glm::vec2& v, float degrees) {
        const float r = glm::radians(degrees);
        const float c = std::cos(r), s = std::sin(r);
        return { c * v.x - s * v.y, s * v.x + c * v.y };
    }

    float DistanceToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
        const glm::vec2 ab = b - a;
        const float t = std::clamp(glm::dot(p - a, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.0f, 1.0f);
        return glm::length(p - (a + ab * t));
    }

    // Sprite'ın dünya uzayındaki köşeleri (rotasyon, ölçek ve origin dahil)
    bool SpriteCorners(const mme::Entity& e, glm::vec2 out[4]) {
        const auto* sprite = e.GetComponent<mme::SpriteComponent>();
        if (!sprite) return false;
        const glm::vec2 size = sprite->size * e.scale;
        const glm::vec2 mn = -sprite->origin * size, mx = (glm::vec2(1.0f) - sprite->origin) * size;
        const glm::vec2 local[4] = { { mn.x, mn.y }, { mx.x, mn.y }, { mx.x, mx.y }, { mn.x, mx.y } };
        for (int i = 0; i < 4; ++i) out[i] = e.position + Rotate(local[i], e.rotation);
        return true;
    }

    bool SpriteContains(const mme::Entity& e, const glm::vec2& world) {
        const auto* sprite = e.GetComponent<mme::SpriteComponent>();
        if (!sprite) return false;
        const glm::vec2 size = sprite->size * e.scale;
        const glm::vec2 a = -sprite->origin * size, b = (glm::vec2(1.0f) - sprite->origin) * size;
        const glm::vec2 local = Rotate(world - e.position, -e.rotation);
        return local.x >= std::min(a.x, b.x) && local.x <= std::max(a.x, b.x) &&
               local.y >= std::min(a.y, b.y) && local.y <= std::max(a.y, b.y);
    }

    void DrawPolygon(const glm::vec2* p, int n, const glm::vec4& color, float thickness) {
        for (int i = 0; i < n; ++i) mme::Renderer2D::DrawLine(p[i], p[(i + 1) % n], color, thickness);
    }

    // Oyun kamerasının göreceği alan
    void CameraFrame(const mme::Entity& e, const mme::CameraComponent& cam, float aspect, glm::vec2 out[4]) {
        const glm::vec2 half{ cam.viewHeight * aspect * 0.5f, cam.viewHeight * 0.5f };
        const glm::vec2 local[4] = { { -half.x, -half.y }, { half.x, -half.y }, { half.x, half.y }, { -half.x, half.y } };
        for (int i = 0; i < 4; ++i) out[i] = e.position + Rotate(local[i], e.rotation);
    }
}

void EditorLayer::FitCameraToGame() {
    // Oyun kamerasının gördüğü alan, kenarlarda biraz boşlukla viewport'a sığsın
    const mme::CameraComponent* cam = m_Scene->GetPrimaryCamera();
    if (!cam) {
        m_Camera.SetPosition({ 0.0f, 0.0f });
        m_Camera.SetZoom(1.0f);
        return;
    }
    const float aspect = m_Project.windowHeight > 0 ? (float)m_Project.windowWidth / (float)m_Project.windowHeight : 16.0f / 9.0f;
    const glm::vec2 frame{ cam->viewHeight * aspect, cam->viewHeight };
    m_Camera.SetPosition(cam->GetEntity().position);
    m_Camera.SetZoom(1.1f * std::max(frame.x / m_ViewportSize.x, frame.y / m_ViewportSize.y));
}

glm::vec2 EditorLayer::ViewportToWorld(const glm::vec2& viewportPx) const {
    return m_Camera.ScreenToWorld(viewportPx);
}

// ---------------------------------------------------------------- pencere
void EditorLayer::DrawViewport() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    const bool open = ImGui::Begin("Sahne");
    ImGui::PopStyleVar();
    if (!open) {
        m_ViewportHovered = m_ViewportFocused = false;
        ImGui::End();
        return;
    }

    DrawToolbar();

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    m_ViewportSize = { std::max(avail.x, 1.0f), std::max(avail.y, 1.0f) };
    if (m_FitCameraFrames > 0 && avail.x > 1.0f && avail.y > 1.0f) {
        FitCameraToGame();
        --m_FitCameraFrames;
    }
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    m_ViewportScreenPos = { pos.x, pos.y };

    // Framebuffer bir önceki frame'in boyutunda olabilir; doku tüm alana gerilir
    ImGui::Image((ImTextureID)(uintptr_t)m_Framebuffer->GetColorTexture(), avail, ImVec2(0, 1), ImVec2(1, 0));
    m_ViewportHovered = ImGui::IsItemHovered();
    m_ViewportFocused = ImGui::IsWindowFocused();

    // Asset'i sahneye bırak: resim -> sprite, prefab -> örnek, sahne -> aç, script -> entity'ye bağla
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload(mme::InspectorUI::kAssetPayload)) {
            const std::string path(static_cast<const char*>(p->Data));
            const ImVec2 m = ImGui::GetMousePos();
            const glm::vec2 world = ViewportToWorld({ m.x - pos.x, m.y - pos.y });
            const std::string lower = [&] { std::string s = path; for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }();
            const std::string ext = fs::path(lower).extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
                mme::Entity& e = CreatePreset("Sprite", world);
                e.SetName(fs::path(path).stem().string());
                auto& sprite = *e.GetComponent<mme::SpriteComponent>();
                sprite.texture = mme::Assets::GetTexture(path);
                const mme::TextureInfo info = mme::Assets::GetTextureInfo(sprite.texture);
                if (info.width > 0) sprite.size = { (float)info.width, (float)info.height };
            } else if (lower.find(".prefab.json") != std::string::npos) {
                BeginEdit();
                if (mme::Entity* e = mme::SceneSerializer::Instantiate(ActiveScene(), path)) {
                    e->position = world;
                    m_Selected = e->GetID();
                }
            } else if (ext == ".json" && !IsPlaying()) {
                OpenScene(path);
            } else if (ext == ".lua" && !IsPlaying()) {
                uint32_t target = PickEntity(world);
                if (!target) target = m_Selected;
                if (mme::Entity* e = m_Scene->FindEntity(target)) {
                    BeginEdit();
                    auto* sc = e->GetComponent<mme::ScriptComponent>();
                    if (!sc) sc = &e->AddComponent<mme::ScriptComponent>();
                    sc->script = path;
                    m_Selected = target;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (!IsPlaying()) UpdateViewportInput();
    else if (m_ViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) ImGui::SetWindowFocus();

    ImGui::End();
}

void EditorLayer::DrawToolbar() {
    ImGui::SetCursorPos(ImVec2(6, 4));
    auto modeButton = [&](const char* label, GizmoMode mode, const char* tip) {
        const bool active = m_GizmoMode == mode;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::SmallButton(label)) m_GizmoMode = mode;
        if (active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
        ImGui::SameLine();
    };
    ImGui::BeginDisabled(IsPlaying());
    modeButton("Taşı", GizmoMode::Move, "W");
    modeButton("Döndür", GizmoMode::Rotate, "E");
    modeButton("Ölçekle", GizmoMode::Scale, "R");
    ImGui::EndDisabled();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (IsPlaying()) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), m_PlayPaused ? "DURAKLATILDI" : "OYUN");
        ImGui::SameLine();
        ImGui::TextDisabled(m_ViewportFocused ? "(girdi oyunda)" : "(girdi için tıklayın)");
    } else {
        ImGui::TextDisabled("zoom %.0f%%   sağ/orta tuş: kaydır   tekerlek: yakınlaş", 100.0f / m_Camera.GetZoom());
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
}

// ---------------------------------------------------------------- girdi
void EditorLayer::UpdateViewportInput() {
    const ImGuiIO& io = ImGui::GetIO();
    const glm::vec2 mousePx{ io.MousePos.x - m_ViewportScreenPos.x, io.MousePos.y - m_ViewportScreenPos.y };
    const glm::vec2 mouse = ViewportToWorld(mousePx);
    mme::Entity* selected = Selected();

    // Yakınlaştırma: imlecin altındaki nokta yerinde kalır
    if (m_ViewportHovered && io.MouseWheel != 0.0f) {
        const float oldZoom = m_Camera.GetZoom();
        const float newZoom = std::clamp(oldZoom * std::pow(1.15f, -io.MouseWheel), 0.02f, 50.0f);
        m_Camera.SetZoom(newZoom);
        m_Camera.SetPosition(mouse - (mouse - m_Camera.GetPosition()) * (newZoom / oldZoom));
    }

    // Kaydırma: sağ ya da orta tuş
    if (m_ViewportHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
        m_Panning = true;
    if (m_Panning) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            const float z = m_Camera.GetZoom();
            m_Camera.SetPosition(m_Camera.GetPosition() + glm::vec2(-io.MouseDelta.x * z, io.MouseDelta.y * z));
        } else {
            m_Panning = false;
        }
    }

    m_HotHandle = (selected && m_DragHandle == GizmoHandle::None && m_ViewportHovered) ? HitGizmo(mouse) : m_HotHandle;
    if (!selected) m_HotHandle = GizmoHandle::None;

    // Tıklama: önce gizmo, sonra seçim (seçilen entity'yi doğrudan sürükleyerek taşı)
    if (m_ViewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImGui::SetWindowFocus();
        GizmoHandle handle = m_HotHandle;
        if (handle == GizmoHandle::None) {
            m_Selected = PickEntity(mouse);
            selected = Selected();
            if (selected && m_GizmoMode == GizmoMode::Move) handle = GizmoHandle::Free;
        }
        if (selected && handle != GizmoHandle::None) {
            BeginEdit();
            m_DragHandle = handle;
            m_DragStartMouse = mouse;
            m_DragStartPos = selected->position;
            m_DragStartRot = selected->rotation;
            m_DragStartScale = selected->scale;
        }
    }

    if (m_DragHandle != GizmoHandle::None) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !selected) {
            m_DragHandle = GizmoHandle::None;
            return;
        }
        const bool snap = io.KeyCtrl;
        const glm::vec2 delta = mouse - m_DragStartMouse;
        const float length = PixelsToWorld(kGizmoLengthPx);

        switch (m_GizmoMode) {
            case GizmoMode::Move: {
                glm::vec2 p = m_DragStartPos;
                if (m_DragHandle != GizmoHandle::Y) p.x += delta.x;
                if (m_DragHandle != GizmoHandle::X) p.y += delta.y;
                if (snap) {
                    if (m_DragHandle != GizmoHandle::Y) p.x = std::round(p.x / m_GridSize) * m_GridSize;
                    if (m_DragHandle != GizmoHandle::X) p.y = std::round(p.y / m_GridSize) * m_GridSize;
                }
                selected->position = p;
                break;
            }
            case GizmoMode::Rotate: {
                const glm::vec2 a = m_DragStartMouse - m_DragStartPos, b = mouse - m_DragStartPos;
                float angle = glm::degrees(std::atan2(b.y, b.x) - std::atan2(a.y, a.x));
                float r = m_DragStartRot + angle;
                if (snap) r = std::round(r / 15.0f) * 15.0f;
                selected->rotation = r;
                break;
            }
            case GizmoMode::Scale: {
                // Yerel eksenler boyunca (entity dönükse eksenler de döner)
                const glm::vec2 ax = Rotate({ 1.0f, 0.0f }, m_DragStartRot), ay = Rotate({ 0.0f, 1.0f }, m_DragStartRot);
                glm::vec2 s = m_DragStartScale;
                if (m_DragHandle == GizmoHandle::X) s.x *= 1.0f + glm::dot(delta, ax) / length;
                else if (m_DragHandle == GizmoHandle::Y) s.y *= 1.0f + glm::dot(delta, ay) / length;
                else s *= std::max(0.01f, 1.0f + (delta.x + delta.y) * 0.5f / length);
                if (snap) s = glm::round(s * 10.0f) / 10.0f;
                selected->scale = s;
                break;
            }
        }
    }
}

EditorLayer::GizmoHandle EditorLayer::HitGizmo(const glm::vec2& mouse) const {
    const mme::Entity* e = m_Scene->FindEntity(m_Selected);
    if (!e) return GizmoHandle::None;
    const glm::vec2 p = e->position;
    const float px = m_Camera.GetZoom();                    // 1 ekran pikseli kaç dünya birimi
    const float length = kGizmoLengthPx * px;
    const float tolerance = 6.0f * px;

    if (m_GizmoMode == GizmoMode::Rotate) {
        return std::abs(glm::length(mouse - p) - length) < tolerance ? GizmoHandle::Ring : GizmoHandle::None;
    }
    const float rot = m_GizmoMode == GizmoMode::Scale ? e->rotation : 0.0f;
    const glm::vec2 d = Rotate(mouse - p, -rot);
    if (std::abs(d.x) < kHandlePx * px * 1.3f && std::abs(d.y) < kHandlePx * px * 1.3f) return GizmoHandle::Free;
    if (DistanceToSegment(d, { 0.0f, 0.0f }, { length, 0.0f }) < tolerance) return GizmoHandle::X;
    if (DistanceToSegment(d, { 0.0f, 0.0f }, { 0.0f, length }) < tolerance) return GizmoHandle::Y;
    return GizmoHandle::None;
}

uint32_t EditorLayer::PickEntity(const glm::vec2& world) const {
    // Önde çizilen (yüksek zLayer, sonra eklenen) önce seçilir
    std::vector<const mme::Entity*> order;
    for (const auto& e : m_Scene->GetEntities())
        if (!e->IsPendingDestroy()) order.push_back(e.get());
    std::stable_sort(order.begin(), order.end(), [](const mme::Entity* a, const mme::Entity* b) { return a->zLayer < b->zLayer; });

    const float px = m_Camera.GetZoom();
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const mme::Entity& e = **it;
        if (SpriteContains(e, world)) return e.GetID();
        bool hit = false;
        bool hasShape = e.HasComponent<mme::SpriteComponent>();
        const_cast<mme::Entity&>(e).ForEachComponent([&](std::type_index, mme::Component& c) {
            if (auto* col = dynamic_cast<mme::Collider2D*>(&c)) { hasShape = true; hit |= col->Contains(world); }
        });
        if (hit) return e.GetID();
        // Görünmez entity'ler (kamera, boş): konumlarındaki simge
        if (!hasShape && glm::length(world - e.position) < 12.0f * px) return e.GetID();
    }
    return 0;
}

// ---------------------------------------------------------------- çizim
void EditorLayer::RenderEditView() {
    mme::Renderer2D::Clear({ 0.13f, 0.14f, 0.17f, 1.0f });
    m_Camera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
    mme::Renderer2D::BeginScene(m_Camera);

    if (m_ShowGrid) DrawGrid();

    mme::SceneDebugDraw& dbg = mme::Scene::GetDebugDraw();
    dbg.colliders = m_ShowColliders;
    dbg.velocities = false;
    dbg.highlightedEntity = 0;
    m_Scene->Render();

    DrawSelectionOverlay();
    DrawGizmo();
    mme::Renderer2D::EndScene();
    dbg.colliders = false;
}

void EditorLayer::DrawGrid() {
    const float zoom = m_Camera.GetZoom();
    float step = m_GridSize;
    while (step / zoom < 8.0f) step *= 4.0f;   // ekranda 8 pikselden sık çizgi olmasın
    const glm::vec2 half = m_Camera.GetVisibleSize() * 0.5f;
    const glm::vec2 c = m_Camera.GetPosition();
    const glm::vec2 mn = c - half, mx = c + half;

    const glm::vec4 minor{ 1.0f, 1.0f, 1.0f, 0.05f }, major{ 1.0f, 1.0f, 1.0f, 0.11f };
    for (float x = std::floor(mn.x / step) * step; x <= mx.x; x += step) {
        const bool isMajor = std::fmod(std::abs(x), step * 4.0f) < 0.5f;
        mme::Renderer2D::DrawLine({ x, mn.y }, { x, mx.y }, isMajor ? major : minor, 1.0f);
    }
    for (float y = std::floor(mn.y / step) * step; y <= mx.y; y += step) {
        const bool isMajor = std::fmod(std::abs(y), step * 4.0f) < 0.5f;
        mme::Renderer2D::DrawLine({ mn.x, y }, { mx.x, y }, isMajor ? major : minor, 1.0f);
    }
    // Dünya eksenleri
    mme::Renderer2D::DrawLine({ mn.x, 0.0f }, { mx.x, 0.0f }, { 0.9f, 0.3f, 0.3f, 0.35f }, 1.5f);
    mme::Renderer2D::DrawLine({ 0.0f, mn.y }, { 0.0f, mx.y }, { 0.3f, 0.9f, 0.3f, 0.35f }, 1.5f);
}

void EditorLayer::DrawSelectionOverlay() {
    const float aspect = m_Project.windowHeight > 0 ? (float)m_Project.windowWidth / (float)m_Project.windowHeight : 16.0f / 9.0f;
    const float px = m_Camera.GetZoom();

    for (const auto& owned : m_Scene->GetEntities()) {
        const mme::Entity& e = *owned;
        if (e.IsPendingDestroy()) continue;
        const bool isSelected = e.GetID() == m_Selected;
        glm::vec2 corners[4];

        if (const auto* cam = e.GetComponent<mme::CameraComponent>()) {
            CameraFrame(e, *cam, aspect, corners);
            DrawPolygon(corners, 4, isSelected ? kSelection : glm::vec4(1.0f, 1.0f, 1.0f, 0.45f), isSelected ? 2.0f : 1.0f);
        }
        const bool visible = e.HasComponent<mme::SpriteComponent>() || e.HasComponent<mme::BoxCollider2D>() || e.HasComponent<mme::CircleCollider2D>();
        if (!visible) {
            // Görünmez entity'leri bulunabilir kıl: küçük artı işareti
            const float s = 8.0f * px;
            const glm::vec4 col = isSelected ? kSelection : glm::vec4(0.8f, 0.8f, 0.9f, 0.6f);
            mme::Renderer2D::DrawLine(e.position - glm::vec2(s, 0), e.position + glm::vec2(s, 0), col, 1.5f);
            mme::Renderer2D::DrawLine(e.position - glm::vec2(0, s), e.position + glm::vec2(0, s), col, 1.5f);
        }
        if (!isSelected) continue;

        if (SpriteCorners(e, corners)) DrawPolygon(corners, 4, kSelection, 2.0f);
        const_cast<mme::Entity&>(e).ForEachComponent([&](std::type_index, mme::Component& c) {
            if (auto* col = dynamic_cast<mme::Collider2D*>(&c)) mme::Scene::DrawColliderOutline(*col, kSelection, 1.5f);
        });
    }
}

void EditorLayer::DrawGizmo() {
    const mme::Entity* e = m_Scene->FindEntity(m_Selected);
    if (!e) return;
    const glm::vec2 p = e->position;
    const float px = m_Camera.GetZoom();
    const float length = kGizmoLengthPx * px;
    const GizmoHandle active = m_DragHandle != GizmoHandle::None ? m_DragHandle : m_HotHandle;
    auto color = [&](GizmoHandle h, const glm::vec4& base) { return active == h ? kHot : base; };

    if (m_GizmoMode == GizmoMode::Rotate) {
        constexpr int kSegments = 48;
        glm::vec2 prev = p + glm::vec2(length, 0.0f);
        for (int i = 1; i <= kSegments; ++i) {
            const float a = glm::two_pi<float>() * (float)i / kSegments;
            const glm::vec2 q = p + length * glm::vec2(std::cos(a), std::sin(a));
            mme::Renderer2D::DrawLine(prev, q, color(GizmoHandle::Ring, kFree), 2.5f);
            prev = q;
        }
        mme::Renderer2D::DrawLine(p, p + Rotate({ length, 0.0f }, e->rotation), kAxisX, 2.0f);
        return;
    }

    const float rot = m_GizmoMode == GizmoMode::Scale ? e->rotation : 0.0f;
    const glm::vec2 ax = Rotate({ length, 0.0f }, rot), ay = Rotate({ 0.0f, length }, rot);
    mme::Renderer2D::DrawLine(p, p + ax, color(GizmoHandle::X, kAxisX), 3.0f);
    mme::Renderer2D::DrawLine(p, p + ay, color(GizmoHandle::Y, kAxisY), 3.0f);

    const float head = kHandlePx * 1.6f * px;
    mme::SpriteDesc tip;
    tip.size = { head, head };
    // Taşı: eşkenar dörtgen uç, ölçekle: kare uç
    tip.rotation = (m_GizmoMode == GizmoMode::Move ? 45.0f : 0.0f) + rot;
    tip.pos = p + ax;
    tip.tint = color(GizmoHandle::X, kAxisX);
    mme::Renderer2D::DrawSprite(tip);
    tip.pos = p + ay;
    tip.tint = color(GizmoHandle::Y, kAxisY);
    mme::Renderer2D::DrawSprite(tip);

    mme::SpriteDesc center;
    center.pos = p;
    center.size = glm::vec2(kHandlePx * 2.0f * px);
    center.rotation = rot;
    center.tint = color(GizmoHandle::Free, kFree);
    center.tint.a = 0.85f;
    mme::Renderer2D::DrawSprite(center);
}
