<div align="center">

# ⚡ QualEngine

**A Modern, Modular, High-Performance C++20 Game Engine & Editor Suite**

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=c%2B%2B&logoColor=white)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/Build-CMake%203.25%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%203.3%2B-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?logo=windows&logoColor=white)](#prerequisites)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Active%20Development-success.svg)](#roadmap)

</div>

---

## 🌟 Overview

**QualEngine** is an open-source, lightweight yet full-featured 3D game engine and editor crafted in modern C++20. Designed with clean architectural separation, data-oriented design principles, and developer ergonomics in mind, QualEngine combines a custom sparse-set ECS, a Physically Based Rendering (PBR) pipeline, Jolt Physics integration, dynamic hot-reloadable C++ scripting, 3D spatial audio, and an integrated editor suite (**Vesta**).

Whether you are developing real-time interactive applications, prototyping gameplay concepts, or exploring modern game engine architecture, QualEngine provides an extensible foundation without unnecessary bloat.

---

## ✨ Key Features

### 🎨 Rendering Architecture
- **Physically Based Rendering (PBR):** Metallic-roughness workflow using Cook-Torrance microfacet specular BRDF and Lambertian diffuse reflection.
- **Directional Shadow Mapping:** Hardware percentage-closer filtering (PCF) and slope-scaled depth bias for smooth contact shadows.
- **Skeletal Mesh Skinning:** GPU-accelerated vertex skinning supporting multi-bone skeletons and animation clips via Assimp.
- **Environment Lighting:** Dynamic skybox mapping and atmospheric height/distance fog.
- **Multi-pass Pipeline:** Decoupled scene, game, skybox, grid, gizmo, and UI render passes with aspect-ratio conformant blitting.

### 🔊 3D Spatial Audio Subsystem
- **Modular Audio Backend:** Core audio abstraction (`IAudioBackend`) isolating platform drivers from high-level gameplay code.
- **Miniaudio Integration:** Single-header, multi-threaded audio execution with low-latency software mixing.
- **3D Spatialization:** Automatic listener orientation and source position tracking with configurable attenuation curves (Inverse, Linear, Exponential) and Doppler roll-off.
- **Format Support:** Native playback for `.wav`, `.mp3`, `.ogg`, and `.flac`.
- **Editor Waveform Preview:** Live playback auditioning directly within the asset inspector.

### ⚡ Dynamic Hot-Reloadable C++ Scripting
- **Zero-Downtime Hot Reloading:** Write gameplay logic in C++ and reload runtime assemblies (`GameModule.dll`) on the fly during edit mode.
- **External Code Editor Integration:** Seamless bidirectional integration with **VS Code** (auto-detected) and **Visual Studio**, or custom external IDEs.
- **Focus Auto-Compilation:** Automatic background recompilation when switching focus back to the editor.
- **High-Level Gameplay API:** Intuitive interfaces for entity queries, input polling, Jolt physics impulse manipulation, and spatial audio triggers.

### 🧩 Custom Sparse-Set ECS & Reflection
- **High-Performance Component Storage:** Cache-efficient contiguous component pools backed by sparse-to-dense entity mappings.
- **SERA Reflection & Serialization:** Compile-time macro reflection for zero-overhead JSON scene persistence and automatic inspector UI generation.
- **Lifecycle Guarantees:** Safe hierarchical parent-child transforms, multi-entity deletions, and scene lifecycle hooks (`on_create`, `on_update`, `on_destroy`).

### 📦 ARIA Asset Pipeline
- **UUID / GUID Tracking:** Persistent asset identification via `.meta` companions independent of file renaming or filesystem moves.
- **Format Ingestion:** Automated loaders and pipelines for 3D models (`.obj`, `.fbx`, `.gltf`), textures (`.png`, `.jpg`, `.tga`, `.dds`), shaders (`.vert`, `.frag`, `.glsl`), audio clips, and material descriptors.
- **Drag-and-Drop Workflow:** Instant assignment of materials, textures, meshes, and audio clips into inspector slots and hierarchy nodes.

### 🖥️ Vesta Editor Suite
- **ImGui Docking Workspace:** Fully customizable panel layouts (`Hierarchy`, `Inspector`, `Scene Viewport`, `Game Viewport`, `Asset Browser`, `Lighting`, `Rendering`).
- **Modular 3D Gizmo System:** Dedicated interactive tools (`IGizmoTool`) for translation, rotation, scale, 2D UI `RectTransform`, light radii, and audio attenuation ranges.
- **Async Task System:** Non-blocking background worker queue with a real-time status overlay for asset loads, script compilation, and project exports.
- **UI Canvas & RectTransform:** Screen-space and world-space 2D UI hierarchy with responsive anchor presets.

### 🚀 Standalone Game Packaging
- **1-Click Game Exporter:** Bundles the standalone player (`GameRuntime.exe`), compiled gameplay assembly (`GameModule.dll`), project assets, engine resources, and `boot.config` into a self-contained shipping directory.

---

## 🏗️ Repository Architecture

```text
qual_engine/
├── CMakeLists.txt             # Root CMake build definition
├── CMakePresets.json          # Standard configure & build presets (VS 2026 / VS 2022)
├── cmake/                     # Modular CMake scripts (compiler flags, dependencies, build config)
├── Application/               # Vesta Editor executable entry point & default workspace
├── GameRuntime/               # Standalone game player executable entry point
├── Projects/
│   └── DemoGame/              # Sample project with C++ gameplay scripts & scenes
├── Engine/                    # Core static library (Engine + Vesta Editor)
│   ├── Async/                 # Multi-threaded background task manager
│   ├── Audio/                 # Miniaudio backend & 3D spatial audio system
│   ├── ECS/                   # Sparse-set entity-component registry & component pools
│   ├── Editor/
│   │   └── Vesta/             # Editor panels, modular gizmo tools, services & viewmodels
│   ├── Engine/
│   │   ├── Assets/            # ARIA asset database, GUID registry, GPU cache & loaders
│   │   └── Scripting/         # Script factory & registration macros
│   ├── Externals/             # Bundled third-party sources (ImGui, Jolt Physics, NFD, nlohmann, stb, miniaudio)
│   ├── Gameplay/              # High-level gameplay API (entity, physics, audio, input wrappers)
│   ├── Graphics/              # Low-level graphics primitives
│   ├── Importers/             # Assimp model/animation loader, texture & cubemap importers
│   ├── Input/                 # Keyboard/mouse input polling & action mapping
│   ├── Project/               # Project lifecycle, standalone exporter & hot-reload module builder
│   ├── Renderer/              # OpenGL PBR pipeline, render passes, materials & shadow mapping
│   ├── Resources/             # Built-in engine shaders, icons & resource path resolver
│   ├── Runtime/               # Standalone runtime loop & layer management
│   ├── Scene/                 # Scene graph, sessions & JSON serialization
│   ├── Scripting/             # Dynamic DLL game module interface
│   ├── Sera/                  # Compile-time reflection & serialization macros
│   └── Systems/               # ECS systems (Render, Physics/Jolt, Animation, Camera, Light)
└── docs/                      # Architecture documentation, Doxygen config & technical reports
```

---

## 🛠️ Prerequisites & Building

### Prerequisites
- **Operating System:** Windows 10 / 11 (x64)
- **Build System:** **CMake 3.25+**
- **Compiler:** MSVC v143+ (**Visual Studio 2022** or **Visual Studio 2026** with C++20 workload)
- **Graphics:** GPU supporting **OpenGL 3.3 Core Profile** or higher
- **External Libraries:** Prebuilt **GLFW**, **GLAD**, **GLM**, and **Assimp** (configurable via CMake cache variables or environment variables: `QUAL_GLFW_ROOT`, `QUAL_GLAD_ROOT`, `QUAL_GLM_ROOT`, `QUAL_ASSIMP_ROOT`, `QUAL_ASSIMP_BUILD_DIR`).

### Building with CMake Presets (Recommended)

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yusufkerman/qual_engine.git
   cd qual_engine
   ```

2. **Configure the project:**
   ```bash
   # For Visual Studio 2026:
   cmake --preset vs2026

   # Or for Visual Studio 2022:
   cmake --preset vs2022
   ```
   *(If your external libraries are installed in custom locations, pass `-DQUAL_GLFW_ROOT=<path>`, etc., or define them in a local `CMakeUserPresets.json`).*

3. **Build the engine, editor, and runtime:**
   ```bash
   # Build all targets (Debug):
   cmake --build --preset debug

   # Or build only the Vesta Editor for fast iteration (Debug):
   cmake --build --preset editor-debug

   # Build optimized Release binaries:
   cmake --build --preset release
   ```

4. **Run the Vesta Editor:**
   Output binaries are placed in `x64/<Config>/`. Launch `x64/Debug/Application.exe` with the working directory set to `Application/`.

### Running in VS Code
The workspace comes preconfigured with CMake tasks and launch profiles:
- Press **`Ctrl + Shift + B`** to build (`Build Debug x64`).
- Open the **Run and Debug** panel (`Ctrl + Shift + D`) and select:
  - **`Debug (x64)`** — Incrementally builds the editor via CMake and launches with the MSVC debugger attached.
  - **`Run (Sadece Çalıştır, Build Yok)`** — Launches `Application.exe` instantaneously without running a build step.
  - **`Debug Full (Editor + GameRuntime)`** — Builds all targets including `GameRuntime.exe` before launching.
  - **`Release (x64)`** — Builds and launches the optimized Release configuration.

---

## 🎮 Quickstart: Creating Your First C++ Script

1. In the **Asset Browser**, right-click inside the `Scripts` folder.
2. Select **[+] Create C++ Script Here...** and enter a class name (e.g., `Rotator`).
3. QualEngine generates `Rotator.h` and `Rotator.cpp` and opens them in your code editor:

```cpp
// Rotator.h
#pragma once
#include <Gameplay/gameplay.h>
#include <Engine/Scripting/script_factory.h>

class Rotator : public gameplay::script
{
public:
    void on_update(float dt) override
    {
        glm::vec3 rot = transform().get_rotation_euler();
        rot.y += 45.0f * dt;
        transform().set_rotation_euler(rot);
    }
};

REGISTER_SCRIPT(Rotator);
```

4. Return to QualEngine. The editor automatically recompiles `GameModule.dll` in the background and hot-reloads your script without restarting!
5. Select an entity in the **Hierarchy** and attach the `Rotator` script from the **Inspector**.

---

## 🗺️ Roadmap

### ✅ Completed Milestones
- [x] Custom Sparse-Set ECS & SERA Compile-Time Reflection
- [x] Cook-Torrance PBR Shader Pipeline & Directional PCF Shadow Mapping
- [x] GPU Skeletal Mesh Skinning & Animation System
- [x] Jolt Physics Integration & Rigidbody/Collider Workflow
- [x] Miniaudio 3D Spatial Audio Subsystem & Inspector Waveform Preview
- [x] Hot-Reloadable C++ Scripting (`GameModule.dll`) & External IDE Integration
- [x] Asynchronous Scene & Asset Loading with Progress Overlay
- [x] Modular Vesta Gizmo Architecture & 1-Click Standalone Game Exporter
- [x] Modern CMake Build System (`CMakePresets.json`)

### 🚀 Upcoming Milestones
- [ ] Precompiled Header (PCH) & Sub-Target Modularization (`QualCore`, `QualRenderer`, `QualEditor`)
- [ ] Vulkan Rendering Backend Abstraction
- [ ] GPU-Driven 2D/3D Particle System
- [ ] Physics Constraints & Dedicated Character Controller Component
- [ ] Built-in CPU/GPU Profiler & Frame Time Visualization Overlay
- [ ] Cross-Platform Linux / macOS Support via CMake FetchContent / vcpkg

---

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

<div align="center">
Crafted with passion for high-performance graphics and game technology.
</div>
