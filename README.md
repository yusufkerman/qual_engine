<div align="center">

# ⚡ QualEngine

**A Modern, Modular, High-Performance C++20 Game Engine & Editor Suite**

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=c%2B%2B&logoColor=white)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%203.3%2B-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?logo=windows&logoColor=white)](#prerequisites)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Status](https://img.shields.io/badge/Status-Active%20Development-success.svg)](#roadmap)

</div>

---

## 🌟 Overview

**QualEngine** is an open-source, lightweight yet full-featured 3D game engine and editor crafted in modern C++20. Designed with clean architectural separation, data-oriented design principles, and developer ergonomics in mind, QualEngine combines a custom sparse-set ECS, a Physically Based Rendering (PBR) pipeline, dynamic hot-reloadable C++ scripting, 3D spatial audio, and an integrated editor suite (**Vesta**).

Whether you are developing real-time interactive applications, prototyping gameplay concepts, or learning modern game engine architecture, QualEngine provides an extensible foundation without unnecessary bloat.

---

## ✨ Key Features

### 🎨 Rendering Architecture
- **Physically Based Rendering (PBR):** Metallic-roughness workflow using Cook-Torrance microfacet specular BRDF and Lambertian diffuse reflection.
- **Directional Shadow Mapping:** Hardware percentage-closer filtering (PCF) and slope-scaled depth bias for smooth contact shadows.
- **Skeletal Mesh Skinning:** GPU-accelerated vertex skinning supporting multi-bone skeletons and animation clips via Assimp.
- **Environment Lighting:** Dynamic skybox mapping and atmospheric height/distance fog.
- **Multi-pass Pipeline:** Decoupled scene and game render targets with aspect-ratio conformant blitting.

### 🔊 3D Spatial Audio Subsystem
- **Modular Audio Backend:** Core audio abstraction (`IAudioBackend`) isolating platform drivers from high-level gameplay code.
- **Miniaudio Integration:** Single-header, multi-threaded audio execution with low-latency software mixing.
- **3D Spatialization:** Automatic listener orientation and source position tracking with configurable attenuation curves (Inverse, Linear, Exponential) and Doppler roll-off.
- **Format Support:** Native playback for `.wav`, `.mp3`, `.ogg`, and `.flac`.
- **Editor Waveform Preview:** Live playback auditioning directly within the asset inspector.

### ⚡ Dynamic Hot-Reloadable C++ Scripting
- **Zero-Downtime Hot Reloading:** Write gameplay logic in C++ and reload runtime assemblies (`GameModule.dll`) on the fly during editor edit mode.
- **External Code Editor Integration:** Seamless bidirectional integration with **VS Code** (auto-detected) and **Visual Studio**, or configure custom external IDEs.
- **Focus Auto-Compilation:** Automatic background recompilation when switching focus back to the engine.
- **High-Level Gameplay API:** Intuitive interfaces for entity queries, input pooling, physics impulse manipulation, and spatial audio triggers.

### 🧩 Custom Sparse-Set ECS
- **High-Performance Component Storage:** Cache-efficient contiguous archetype arrays backed by sparse-to-dense mappings.
- **SERA Reflection & Serialization:** Compile-time macro reflection for zero-overhead JSON scene persistence and inspector introspection.
- **Lifecycle Guarantees:** Safe hierarchical parent-child transforms, multi-entity deletions, and scene lifecycle hooks (`on_create`, `on_update`, `on_destroy`).

### 📦 ARIA Asset Pipeline
- **UUID / GUID Tracking:** Persistent asset identification independent of file renaming or filesystem moves.
- **Format Ingestion:** Automated loaders and pipelines for 3D models (`.obj`, `.fbx`, `.gltf`), textures (`.png`, `.jpg`, `.tga`, `.dds`), shaders (`.glsl`), audio clips, and material descriptors (`.amat`).
- **Drag-and-Drop Workflow:** Instant assignment of materials, textures, and audio clips into inspector slots and hierarchy nodes.

### 🖥️ Vesta Editor Suite
- **ImGui Docking Workspace:** Fully customizable panel layouts with persistence.
- **Interactive Viewports:** Guizmo 3D translation/rotation/scale manipulators, grid displays, and camera fly-through navigation.
- **Async Task System:** Non-blocking background worker queue featuring a real-time progress bar for asset loads, compilation, and exports.
- **UI Canvas & RectTransform:** Full screen-space and world-space 2D UI hierarchy with responsive anchor presets (Center, Top-Left, Stretch).

### 🚀 Standalone Standalone Packaging
- **1-Click Game Exporter:** Bundles compiled player runtimes (`GameRuntime.exe`), dynamic gameplay assemblies, packed assets, and `boot.config` into standalone shipping builds.

---

## 🏗️ Architecture Overview

```
QualEngine/
├── Application/               # Vesta Editor entry point and desktop launcher
├── Engine/
│   ├── Async/                 # Multi-threaded background tasks & loading status
│   ├── Audio/                 # miniaudio backend, audio system & spatial sound
│   ├── Core/                  # Math utilities, memory allocators, core primitives
│   ├── ECS/                   # Sparse-set entity component system & components
│   ├── Editor/                # Vesta UI panels, inspectors, viewports & viewmodels
│   ├── Engine/
│   │   └── Assets/            # ARIA asset management, pipeline & loaders
│   ├── Gameplay/              # Scriptable entity base, input & gameplay API
│   ├── Project/               # Project management, exporter, engine preferences
│   ├── Renderer/              # OpenGL PBR pipeline, shaders, meshes, shadow maps
│   ├── Runtime/               # Standalone game runtime player (GameRuntime)
│   ├── Scene/                 # Scene graph, hierarchy & JSON serialization
│   ├── Scripting/             # Dynamic module loader, scaffolding & compiler
│   └── Sera/                  # Compile-time reflection and serialization macros
```

---

## 🛠️ Prerequisites & Building

### Prerequisites
- **Operating System:** Windows 10 / 11 (x64)
- **Compiler:** MSVC v143+ (Visual Studio 2022 or 2026 recommended) with **C++20** support
- **Graphics:** GPU supporting **OpenGL 3.3 Core Profile** or higher

### Building from Source
1. Clone the repository:
   ```bash
   git clone https://github.com/yusufkerman/my_game_engine.git QualEngine
   cd QualEngine
   ```
2. Open the solution file:
   - Double click `QualEngine.slnx` or open it with Visual Studio.
3. Select **Debug** or **Release** configuration and **x64** platform.
4. Build the solution (`Ctrl + Shift + B`).
5. Run the editor (`Application` project) to begin creating projects.

---

## 🎮 Quickstart: Creating Your First Script

1. In the **Asset Browser**, right-click inside the `Scripts` folder.
2. Select **[+] Create C++ Script Here...** and enter a class name (e.g., `Rotator`).
3. QualEngine generates both `Rotator.h` and `Rotator.cpp` and automatically opens them in your configured code editor:

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

4. Return to QualEngine. The engine automatically recompiles the module in the background without restarting the editor!
5. In the **Hierarchy**, select an entity and attach the newly registered script component from the **Inspector**.

---

## 🗺️ Roadmap & Current Status

QualEngine is under active continuous development. Upcoming milestones include:
- [x] Cook-Torrance PBR Shader Pipeline
- [x] Asynchronous Scene & Asset Loading with Modern Status Widget
- [x] Miniaudio 3D Spatial Audio Subsystem
- [x] Asset Browser Code Generation & External IDE Launcher
- [ ] Vulkan Rendering Backend Abstraction
- [ ] Integrated 2D/3D Particle System
- [ ] Physics Constraints & Character Controller
- [ ] Profiler & Frame Time Visualization Overlay

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

<div align="center">
Crafted with passion for high-performance graphics and game technology.
</div>

