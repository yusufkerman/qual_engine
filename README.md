<div align="center">

# ⚡ QualEngine

**A High-Performance, Modular C++20 Game Engine & Vesta Editor Suite**

[![Release](https://img.shields.io/badge/Release-v0.2.0--alpha-blue.svg?logo=github&logoColor=white)](https://github.com/yusufkerman/qual_engine/releases)
[![Tests](https://img.shields.io/badge/Tests-26%2F26%20Passing%20(100%25)-brightgreen.svg?logo=checkmarx&logoColor=white)](#automated-test-pipeline)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=c%2B%2B&logoColor=white)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/Build-CMake%203.25%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%204.5%20Core-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
[![ECS](https://img.shields.io/badge/ECS-EnTT%20v3.13.2-FF6F00?logo=cplusplus&logoColor=white)](https://github.com/skypjack/entt)
[![Physics](https://img.shields.io/badge/Physics-Jolt%20Physics-00C853)](https://github.com/jrouwe/JoltPhysics)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?logo=windows&logoColor=white)](#prerequisites)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

</div>

---

## 🌟 Overview

**QualEngine** is an open-source, data-oriented 3D game engine and editor crafted in modern C++20 for high-performance interactive applications. Built on industry-standard foundations, QualEngine pairs the battle-tested **EnTT Sparse-Set ECS** architecture with a modern **OpenGL 4.5 Core** PBR renderer, **Jolt Physics**, **Miniaudio 3D spatial sound**, **dynamic C++ script hot-reloading**, a **Win32 SEH crash barrier**, and an integrated docking editor suite (**Vesta**).

QualEngine features a clean dual-executable architecture:
* **`Application.exe` (Vesta Editor):** The full development environment featuring multi-viewport docking, asset inspection, runtime debugging, and live scene manipulation.
* **`GameRuntime.exe` (Standalone Player):** A lightweight, editor-free execution binary that loads exported game assets, compiled gameplay assemblies, and scenes to ship self-contained, optimized player builds.

---

## ✨ Architectural Pillars & Features

### 🧩 Industry-Standard ECS Architecture (EnTT v3.13.2)
- **Sparse-Set Storage:** High-performance, cache-aligned contiguous memory arrays for components, guaranteeing $O(1)$ operations with zero pointer-chasing overhead.
- **Generational Entity Identity:** Safe entity recycling with 32-bit generational identifiers preventing dangling references.
- **Backward-Compatible Proxy Layer:** Seamless range-for support (`for (auto& [e, comp] : reg.get_all<T>())`) enabling in-place component mutation without memory copies.
- **Multi-Component Acceleration:** Native EnTT query views (`reg.view<T1, T2>()`) optimizing render and physics batch traversal directly at L1/L2 cache speeds.
- **SERA Reflection & Serialization:** Compile-time macro reflection for two-way JSON scene serialization and automatic Vesta Inspector binding.

### 🎨 Modern Graphics Pipeline (OpenGL 4.5 Core)
- **Physically Based Rendering (PBR):** Metallic-roughness workflow implementing Cook-Torrance microfacet specular BRDF and Lambertian diffuse response.
- **Uniform Buffer Objects (UBO):** High-speed `std140` memory buffer (`LightBuffer`) updating scene light batches via GPU memory blocks in a single draw-call setup.
- **6-Plane Gribb-Hartmann Frustum Culling:** Submesh-level bounding sphere culling for both static meshes and skeletal models, discarding out-of-view objects before draw dispatch with real-time editor telemetry.
- **Cascaded Shadow Maps (CSM):** Multi-cascade frustum splitting (15m, 50m, 150m) utilizing `GL_TEXTURE_2D_ARRAY` and texel stabilization, delivering sharp, artifact-free directional shadows across expansive worlds with 9-sample PCF filtering.
- **Directional PCF Shadows:** Percentage-Closer Filtering with slope-scaled depth bias on custom Framebuffer Objects (FBO).
- **Uniform Location Caching:** Zero string lookups during render dispatch via pre-cached GPU uniform location tables.
- **GPU Skeletal Mesh Skinning:** Real-time multi-bone skeletal animation with runtime crossfading and bone attachment tracking.
- **OpenGL 4.5 Driver Diagnostics:** Driver-level `glDebugMessageCallback` logging with categorized severity filtering.

### 🛡️ Crash Resilience & Diagnostics Engine
- **SEH Script Crash Barrier:** Win32 Structured Exception Handling boundary intercepting hardware faults (Access Violations, division by zero) inside native user scripts (`script_invoker.h`).
- **Component Quarantine:** Faulting scripts are automatically disabled and quarantined without crashing the editor, allowing developers to inspect errors and fix code live.
- **Zero-Heap Ring Buffer Logger:** High-throughput, thread-safe memory logger with spam deduplication and console output.
- **Vesta Developer Console:** Integrated dockable log viewer with level filtering (Info, Warn, Error), live search, autoscroll, and stack trace inspection.
- **5-Layer CRT/SEH Minidump Generator:** Unhandled crashes produce crash minidumps (`.dmp`) and diagnostic text reports (`.txt`) for post-mortem debugging.

### ⚡ Dynamic Hot-Reloadable C++ Scripting
- **Zero-Downtime Hot Reloading:** Compile gameplay assemblies (`GameModule.dll`) on the fly while the editor is running.
- **External IDE Integration:** Automatic project scaffolding and auto-detection for **VS Code** and **Visual Studio**.
- **Focus Auto-Compilation:** Background compilation triggered seamlessly when switching focus back to Vesta.
- **Script State Preservation:** Active member variables and state are serialized and preserved across live DLL reloads and scene serialization.

### 🔊 3D Spatial Audio & Jolt Physics
- **Jolt Physics Integration:** Multi-threaded rigid body simulation, collision event queues, and compound shapes.
- **Kinematic Character Controller:** Jolt `CharacterVirtual` kinematic controller supporting smooth obstacle stepping, max slope clamping, jumping, and ground contact resolution.
- **Miniaudio Subsystem:** Low-latency 3D spatialized audio with distance attenuation (Inverse, Linear, Exponential) and live waveform inspection.

---

### ⏪ Vesta Undo / Redo Architecture (Ctrl+Z / Ctrl+Y)
- **Universal Transaction Stack:** Full command pattern integration (`command_stack`) with configurable history depth.
- **Gizmo Tool Hooks:** Interactive translation, rotation, and scale gizmo operations are recorded as undoable commands upon mouse release.
- **Hierarchy Commands:** Entity creation and deletion can be reversed at any time, restoring full component states from memory snapshots.

### 📦 Virtual File System (VFS) & `data.pak` Security
- **QPAK Binary Packaging:** Zero-dependency `QPAK` archive container format featuring 0x5A XOR obfuscation, Adler32/CRC verification, and memory offset tables.
- **Transparent RAM Streaming:** Textures, 3D meshes, audio, and scenes stream directly from memory buffers in release builds, shielding raw game assets from tampering.
- **1-Click Standalone Packager:** Exporter packages assets directly into `data.pak` alongside the `GameRuntime.exe` binary.

---

## 🧪 Automated Test Pipeline (doctest v2.5.0)

QualEngine includes a fully automated test harness with **26 test cases** and **223 assertions** covering core engine systems:

```bash
# Run automated tests
cmake --build --preset debug --target QualTests
.\x64\Debug\QualTests.exe
```

| Subsystem Suite | Test Cases | Assertions | Status |
| :--- | :---: | :---: | :---: |
| **Undo / Redo System** | 3 | 18 | **PASSED (100%)** |
| **Kinematic Character Controller** | 4 | 22 | **PASSED (100%)** |
| **Virtual File System (VFS)** | 4 | 24 | **PASSED (100%)** |
| **Cascaded Shadow Maps (CSM)** | 2 | 12 | **PASSED (100%)** |
| **Script State Preservation** | 3 | 27 | **PASSED (100%)** |
| **EnTT ECS Sparse-Set** | 5 | 58 | **PASSED (100%)** |
| **Transform Hierarchy** | 3 | 32 | **PASSED (100%)** |
| **Script Crash Boundary (SEH)** | 2 | 30 | **PASSED (100%)** |
| **Total** | **26** | **223** | **100% SUCCESS** |

---

## 🏗️ Dual-Executable Architecture: Editor vs Player

QualEngine cleanly separates the authoring pipeline from the end-user player runtime:

```
QualEngine Distribution/
├── Application.exe       # [VESTA EDITOR]
│                         # Full authoring suite with docking workspace, scene gizmos,
│                         # asset browser, inspector, and project management.
│
├── GameRuntime.exe       # [STANDALONE PLAYER]
│                         # Lightweight game runner stripped of all editor panels and ImGui.
│                         # Used by Vesta's 1-Click Exporter to create final game builds.
│
├── assimp-vc145-mt.dll   # Shared 3D model asset import runtime
└── EngineResources/      # Core shaders, PBR textures, fonts, and internal resources
```

When you click **Export Game** in Vesta, the engine copies `GameRuntime.exe` to your shipping folder, renames it to your project name, and packages your compiled `GameModule.dll` and assets alongside it.

---

## 📦 Download & Installation

Prebuilt standalone binaries are available on the [Releases](https://github.com/yusufkerman/qual_engine/releases) page:

1. **Windows Installer (`QualEngine-v0.1.0-alpha-Setup.exe`):**
   - Automated setup wizard with Start Menu shortcuts, desktop icons, and uninstaller.
2. **Portable ZIP (`QualEngine-v0.1.0-alpha-win64.zip`):**
   - Extract and run `Application.exe` directly without installation.

---

## 🛠️ Building from Source

### Prerequisites
- **Operating System:** Windows 10 / 11 (x64)
- **Build System:** **CMake 3.25+**
- **Compiler:** MSVC v143+ (**Visual Studio 2022** or **Visual Studio 2026** with C++20 tools)
- **Graphics Card:** GPU with **OpenGL 4.5 Core Profile** support
- **Dependencies:** Prebuilt **GLFW**, **GLAD**, **GLM**, and **Assimp** (paths configurable via CMake cache or environment variables: `QUAL_GLFW_ROOT`, `QUAL_GLAD_ROOT`, `QUAL_GLM_ROOT`, `QUAL_ASSIMP_ROOT`, `QUAL_ASSIMP_BUILD_DIR`).

### Quick Build with CMake Presets

```bash
# 1. Clone repository
git clone https://github.com/yusufkerman/qual_engine.git
cd qual_engine

# 2. Configure project (Visual Studio 2026 or 2022)
cmake --preset vs2026

# 3. Build optimized Release binaries
cmake --build --preset release

# 4. Or build Debug for editor development
cmake --build --preset debug
```

Output binaries are placed in `x64/<Config>/` (`Application.exe`, `GameRuntime.exe`, `Engine.lib`).

---

## 🚀 Creating Release Packages (Setup.exe & ZIP)

QualEngine includes an automated packaging pipeline script under `scripts/`:

```powershell
# Build Release and generate both the Portable ZIP and Inno Setup installer:
.\scripts\package_release.ps1 -Version "0.1.0-alpha"
```

The script stages release files into `dist/staging/` and generates:
- `dist/QualEngine-v0.1.0-alpha-win64.zip` (Portable Archive)
- `dist/QualEngine-v0.1.0-alpha-Setup.exe` (Windows Installer via Inno Setup)

*(To generate the installer executable, install Inno Setup 6 via `winget install JRSoftware.InnoSetup`).*

---

## 🎮 Quickstart: Writing Gameplay Logic

1. Inside Vesta Editor's **Asset Browser**, right-click in `Scripts` and select **[+] Create C++ Script Here...**.
2. Name your class `Rotator`. QualEngine scaffolds the script:

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

3. Return to QualEngine. `GameModule.dll` automatically recompiles in the background and hot-reloads without restarting the editor.
4. If a script produces an access violation, the **SEH Crash Barrier** intercepts it, disables the component, and displays the callstack in the Vesta Console.

---

## 🗺️ Roadmap & Current State

### ✅ Implemented (v0.1.0-alpha)
- [x] EnTT v3.13.2 Sparse-Set ECS Architecture (Zero-breaking, 22/22 components)
- [x] Cook-Torrance PBR Shader Pipeline & std140 Light UBO
- [x] 6-Plane Gribb-Hartmann Frustum Culling & Live Telemetry
- [x] GPU Skeletal Mesh Skinning & Animation System
- [x] Jolt Physics Multi-Threaded Simulation
- [x] Dynamic Hot-Reloadable C++ Scripting (`GameModule.dll`)
- [x] Win32 SEH Crash Barrier & Script Quarantine Protection
- [x] Zero-Heap Memory Logger & Vesta Developer Console Panel
- [x] OpenGL 4.5 Core Debug Callback Driver Hook
- [x] 5-Layer CRT/SEH Minidump Post-Mortem Crash Reporting
- [x] Dual-Executable Architecture (`Application.exe` + `GameRuntime.exe`)
- [x] Automated Packaging Pipeline (`Setup.exe` & Portable `.zip`)

### 📌 Upcoming Milestones
- [ ] VFS (Virtual File System) & Encrypted Asset Package Archive (`.pak`)
- [ ] Cascaded Shadow Maps (CSM) for large exterior scenes
- [ ] Vulkan Rendering Backend Abstraction
- [ ] Dedicated Character Controller & Physics Constraints
- [ ] Linux / macOS Cross-Platform Support

---

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

<div align="center">
<b>QualEngine</b> — Engineered for high performance, modularity, and crash resilience.
</div>
