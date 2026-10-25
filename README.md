# MakeMyEngine

## 🔹 Short Summary

MakeMyEngine is a lightweight 2D game engine/skeleton built with modern C++ and OpenGL. It combines tilemaps, sprite rendering, AABB collisions, a simple UI, and input handling to help you go from rapid prototyping to production.

---
<img width="1322" height="798" alt="Ekran görüntüsü 2025-11-05 175245" src="https://github.com/user-attachments/assets/d850f2b9-a93f-470b-9d51-34506f431e97" />

## 🔹 Why

* Reduce "setup from scratch" overhead for hobby/prototype projects
* Provide a learnable, readable, and extensible 2D engine skeleton
* Separate rendering, scene, collision, and input into clear, modular systems

---

## 🔹 Technologies and Usage (approx.)

* **C++ (core logic and architecture) — ~70%**
  Performance, memory control, portability, mature ecosystem.
* **OpenGL + GLSL (render pipeline and shaders) — ~15%**
  Broad support, great for 2D + learning.
* **GLFW (window/context, input) — ~7%**
  Minimal deps, cross-platform.
* **stb_image (texture loading) — ~3%**
  Single-header, zero deps.
* **CMake (build system) — ~5%**
  Cross-platform builds, IDE integration.

---

## 🔹 Highlights

* **Engine / game split:** the engine is a static library (`MME::Engine`); games are separate executables that never see OpenGL or GLFW headers
* **Application + layers:** `Application` owns the window and main loop; games plug in as `Layer`s (`OnFixedUpdate`, `OnUpdate`, `OnRender`, `OnUIRender`, `OnImGuiRender`, `OnEvent`); layers can be pushed/popped at runtime
* **Fixed-timestep loop:** simulation at 60 Hz, rendering at display rate; pause, single-step and time scale
* **Batched 2D renderer:** `Renderer2D` packs up to 10,000 quads and 8 textures into a single draw call; built-in shader embedded in the engine; debug lines/outlines
* **Scene + components:** `Scene` owns `Entity`s, entities own components (`SpriteComponent`, `SpriteAnimator`, `ParticleEmitter`, `RigidBody2D`, `BoxCollider2D`, your own)
* **Reflection:** `ComponentRegistry` describes component fields once; the debug inspector and the JSON serializer are both driven by it
* **2D physics:** static / kinematic / dynamic bodies, spatial-hash broadphase, substepping against tunneling, restitution, gravity, `OnCollisionEnter` / `OnTriggerEnter`
* **Scene serialization:** `SceneSerializer` saves/loads scenes as JSON (component order preserved, textures stored by path)
* **Debug tools:** Dear ImGui panel (F1) with hierarchy, live inspector, add/remove components, collider and velocity drawing, physics/audio stats; stats overlay (F3)
* **Audio:** miniaudio-based sound effects (pitch/volume), streamed music, master / SFX / music volume
* **Text:** TTF fonts via stb_truetype with UTF-8 (full Turkish support: ç ğ ı ö ş ü İ), plus a built-in 5x7 bitmap font
* **Sprites & effects:** sprite-sheet animation clips, pooled particle system
* **Hot-reload:** in Debug builds assets are read straight from the repo; edited textures reload live under the same `TextureID`
* **Ortho camera, CSV tilemaps, event-driven input, immediate-mode UI buttons**
* **Tooling:** leveled `Log`, `MME_ASSERT`, `vcpkg.json` manifest, CMake presets, 32 automated tests

---

## 🔹 Architecture at a Glance

```
engine/                 MME::Engine (static library)
  include/MME/          public API — #include <MME/MME.h>
  src/                  implementation (OpenGL, GLFW, ImGui backends, miniaudio, stb stay in here)
games/
  PingPong/             two-player Pong: sounds, particles, Turkish UI
  TilemapDemo/          tilemap, camera, animated sprites, save/load scene
tests/
  EngineTests.cpp       unit tests (no GPU needed)
  AppTests.cpp          main-loop tests on a hidden window (needs a GPU)
assets/                 textures, maps, fonts, sounds, music, sprites
```

* **Frame:** poll events → fixed updates (physics) → update → render world → render UI
* **Rendering path:** camera → `Renderer2D::BeginScene` → batched quads → one draw call per batch
* **Physics step:** pick substeps → integrate bodies → broadphase pairs → resolve AABB overlaps → dispatch contact events

### Minimal game

```cpp
#include <MME/MME.h>

class MyLayer : public mme::Layer {
    mme::Scene m_Scene;
    mme::OrthoCamera m_Camera{ 1280, 720 };
public:
    void OnAttach() override {
        auto& box = m_Scene.CreateEntity("Box");
        box.AddComponent<mme::SpriteComponent>().size = { 64, 64 };
        box.AddComponent<mme::BoxCollider2D>().size = { 64, 64 };
        box.AddComponent<mme::RigidBody2D>().velocity = { 100, 0 };
    }
    void OnFixedUpdate(float step) override { m_Scene.FixedUpdate(step); }
    void OnRender() override {
        mme::Renderer2D::BeginScene(m_Camera);
        m_Scene.Render();
        mme::Renderer2D::EndScene();
    }
};

int main() {
    mme::Application app;
    app.PushLayer<MyLayer>();
    app.Run();
}
```

Register it in CMake with `mme_add_game(MyGame src/main.cpp)`.

---

## 🔹 Roadmap

See [ENGINE_ROADMAP.md](ENGINE_ROADMAP.md) for the full plan and known limitations. Next up:

* [ ] Platform validation (Linux/macOS)
* [ ] Rotated boxes and circle colliders
* [ ] In-engine scene editor on top of the ImGui panel

---

## 🔹 Build & Run (Windows / VS 2022)

Requires [vcpkg](https://github.com/microsoft/vcpkg) and the `VCPKG_ROOT` environment variable pointing at it. Dependencies (`glad`, `glfw3`, `glm`, `imgui`, `miniaudio`, `nlohmann-json`, `stb`) are declared in `vcpkg.json` and installed automatically on the first configure.

```
cmake --preset vs2022
cmake --build --preset debug
ctest --preset debug
```

Executables land in `build/bin/Debug/` (`PingPong.exe`, `TilemapDemo.exe`, `EngineTests.exe`, `EngineAppTests.exe`) with `assets/` copied next to them.
Debug builds read assets directly from the repository's `assets/` folder (for hot-reload); Release builds use the copied folder.
To use a vcpkg path without setting `VCPKG_ROOT`, add a git-ignored `CMakeUserPresets.json` that inherits `vs2022` and sets it in `environment`.

**PingPong:** `W`/`S` left paddle · `↑`/`↓` right paddle · `Space` serve · `M` music · `Enter`/`R` restart · `Esc` quit

**TilemapDemo:** `WASD` camera · wheel zoom · left click paint tile · right click erase · `F5` save scene · `F9` load scene

**Everywhere:** `F1` debug panel · `F3` stats overlay

### Assets & credits

* `assets/fonts/FiraSans-Regular.ttf`: Fira Sans by Mozilla / Telefonica, [SIL Open Font License](assets/fonts/OFL.txt)
* Sound effects, music loop and the coin sprite sheet are generated procedurally for this project

---

## 🔹 Contributing

Issues + PRs welcome.
Focus: readability + modularity.

---

## 🔹 License

See **LICENSE**.

---

## 🔹 Non‑GitHub Promo (LinkedIn / X / Blog)

### **MakeMyEngine: A lightweight 2D game engine skeleton in C++ and OpenGL**

Want to build your own engine but not sure where to start?
MakeMyEngine is a modern C++ + OpenGL 2D engine/skeleton that’s easy to learn and extend. It includes tilemaps, sprite rendering, AABB collision, input handling, and a simple UI.

### ✅ What’s inside?

* Ortho camera + controller
* Batched 2D renderer (one draw call per batch), sprites, GLSL shader pipeline
* CSV-based tilemaps
* AABB collision system
* Input + simple UI layer
* Clean component‑based structure

<img width="1316" height="797" alt="Ekran görüntüsü 2025-11-05 175211" src="https://github.com/user-attachments/assets/b498fe91-9c7d-46b9-b51e-b75c77f92100" />

### ✅ Why these technologies?

* C++ → performance, control, portability
* OpenGL + GLSL → widely supported, flexible
* GLFW → lightweight, cross‑platform
* stb_image → zero‑dependency texture loading
* CMake → consistent builds

### ✅ Who is it for?

* Students + hobbyists wanting fast prototyping
* Developers learning engine design
* Anyone who prefers readable + modular architecture

### ✅ Next

Linux/macOS support, more collider shapes, an in-engine scene editor.

### ✅ Try it

Open with CMake → build → run.
Assets under `/assets` (textures, tilemaps); see Build & Run above.

---
