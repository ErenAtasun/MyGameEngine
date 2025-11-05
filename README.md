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

* **Ortho camera + controller:** `OrthoCamera`, `OrthoCameraController`
* **2D renderer + sprite system:** `Renderer2D`, `SpriteComponent`, GLSL shaders (`assets/shaders`)
* **CSV-based tilemaps:** `Tilemap` (`assets/maps`)
* **Physics/collision:** AABB (`AABBCollider`, `CollisionSystem`)
* **Input handling:** `Input` (keyboard)
* **Simple UI layer:** `UI`
* **Entity/Component architecture:** `Entity`, `Component`
* **Asset management:** `AssetManager`

---


## 🔹 Architecture at a Glance

<img width="263" height="733" alt="Ekran görüntüsü 2025-11-05 185634" src="https://github.com/user-attachments/assets/e88be025-75f9-42c2-be23-b0d7e5e8e179" />
* **Scene flow:** `Scene` + `GameScene` manage game loop + layers
* **Component-driven design:** focused components (`SpriteComponent`, `VelocityComponent`, `BallComponent`, `PaddleController`)
* **Rendering path:** Ortho camera → `Renderer2D` → shader → texture
* **Assets:** under `/assets` (shaders, textures, CSV tilemaps)

---

## 🔹 Roadmap

* [ ] Texture batching + atlas optimizations
* [ ] Simple particle system
* [ ] Audio (OpenAL or miniaudio)
* [ ] Scripting (Lua/AngelScript)
* [ ] Basic UI widgets (button, slider, text input)
* [ ] Platform layer validation (Linux/macOS)

---

## 🔹 Build & Run (Windows / VS)

1. Open project with **CMake**
2. Generate Visual Studio solution
3. Build + run `MakeMyEngine.exe`

> Build outputs + example assets live under `build/Debug`.

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
* 2D renderer, sprites, GLSL shader pipeline
* CSV-based tilemaps
* AABB collision system
* Input + simple UI layer
* Clean component‑based structure
* 
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

Batching, particles, audio, scripting.

### ✅ Try it

Open with CMake → build → run.
Assets under `/assets` (shaders, textures, tilemaps).

---
