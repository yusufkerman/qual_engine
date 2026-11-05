# QualEngine

QualEngine is a modular 3D game engine and integrated editor (**Vesta**) written in C++20 for Windows x64. It separates the authoring environment (`Application.exe`) from the standalone shipping runtime (`GameRuntime.exe`) and supports both a physically based rendering (PBR) pipeline and a retro PlayStation 1 (PSX) rendering mode.

---

## Core Technologies

- **Language & Standard:** C++20 (MSVC v143+)
- **Build System:** CMake 3.25+
- **Graphics API:** OpenGL 4.5 Core Profile (GLSL `#version 330 core` / `#version 450 core`)
- **Rendering:**
  - Cook-Torrance metallic-roughness PBR workflow & HDR (`RGBA16F`) pipeline
  - Cascaded Shadow Maps (CSM), spot light shadows, and omnidirectional point light shadows
  - Screen-Space Ambient Occlusion (SSAO / GTAO) & Screen-Space Global Illumination (SSGI)
  - Volumetric lighting (raymarched god rays) and exponential distance/height fog
  - Post-processing: Tone mapping (ACES, AgX, Reinhard, Uncharted 2), TAA (Halton jitter), and FXAA 3.11
  - PS1 retro mode (vertex snapping, affine texture mapping, RGB555 color quantization, 4x4 Bayer dithering)
  - GPU skeletal mesh skinning and animation blending
- **Architecture & Subsystems:**
  - Sparse-set Entity Component System (ECS) with compile-time reflection (`SERA`) and JSON serialization
  - Multi-threaded rigid body physics, kinematic character controller, and mesh/convex colliders
  - Dynamic C++ gameplay scripting (`GameModule.dll`) with live hot-reloading and Win32 SEH crash isolation
  - Virtual File System (VFS) with binary `.pak` archiving (`QPAK`) for standalone builds

---

## Dependencies & Third-Party Libraries

### External Libraries (Prebuilt / System)
| Library | Purpose |
| :--- | :--- |
| **GLFW** | Window creation, OpenGL context management, and input handling |
| **GLAD** | OpenGL 4.5 Core function loader |
| **GLM** | Header-only linear algebra and mathematics library |
| **Assimp** | 3D model and skeletal animation importing (glTF 2.0, FBX, OBJ, DAE) |

### Bundled Libraries (`Engine/Externals`)
| Library | Purpose |
| :--- | :--- |
| **EnTT** | Sparse-set Entity Component System (ECS) |
| **Jolt Physics** | Multi-threaded 3D rigid body physics and collision detection |
| **Dear ImGui & ImGuizmo** | Vesta Editor docking UI and 3D viewport manipulation gizmos |
| **miniaudio** | Single-header 3D spatial audio playback and mixing |
| **nlohmann/json** | Scene, project, and material JSON serialization |
| **stb** | Texture loading (`stb_image`) and font rasterization (`stb_truetype`) |
| **stduuid** | RFC-4122 UUID generation for asset tracking (`Aria`) |
| **Native File Dialog (NFD)** | OS-native file and directory selection dialogs |
| **doctest** | Unit and integration testing framework |

---

## Prerequisites & Setup

### System Requirements
- **OS:** Windows 10 / 11 (x64)
- **Compiler:** Visual Studio 2022 or 2026 (Desktop development with C++)
- **CMake:** 3.25 or newer
- **GPU:** OpenGL 4.5 Core compatible graphics card

### Configuring External Dependency Paths
CMake locates `GLFW`, `GLAD`, `GLM`, and `Assimp` via cache variables or environment variables. If your libraries are installed in non-default locations, specify them during configuration:

| Variable | Default Path | Description |
| :--- | :--- | :--- |
| `QUAL_GLFW_ROOT` | `C:/Libraries/glfw` | Contains `include/` and `lib-vc2022/` |
| `QUAL_GLAD_ROOT` | `C:/Libraries/glad` | Contains `include/` and `src/glad.c` |
| `QUAL_GLM_ROOT` | `C:/Libraries/glm` | Contains `glm/glm.hpp` |
| `QUAL_ASSIMP_ROOT` | `C:/dev/assimp-6.0.5` | Contains `include/assimp/` |
| `QUAL_ASSIMP_BUILD_DIR` | `C:/dev/assimp-6.0.5/assimp_build` | Contains compiled `lib/` and `bin/` |

---

## Building from Source

```powershell
# 1. Clone the repository
git clone https://github.com/yusufkerman/qual_engine.git
cd qual_engine

# 2. Configure with CMake preset
cmake --preset vs2026

# 3. Build Release or Debug binaries
cmake --build --preset release
cmake --build --preset debug
```

Compiled binaries are output to `x64/Release/` or `x64/Debug/`:
- `Application.exe` — Vesta Editor
- `GameRuntime.exe` — Standalone game runner
- `Engine.lib` — Core engine static library
- `QualTests.exe` — Automated unit test runner

---

## Usage

### 1. Running the Editor
Launch `x64/Release/Application.exe` (or `x64/Debug/Application.exe`). From the startup dialog, create a new project or open an existing project directory.

### 2. Writing C++ Gameplay Scripts
Gameplay scripts are compiled into a dynamic library (`GameModule.dll`) and hot-reloaded by the editor without restarting.

1. In the **Asset Browser** panel, navigate to the `Scripts` folder, right-click, and select **Create C++ Script**.
2. Implement your component behavior by inheriting from `gameplay::script`:

```cpp
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

3. Save the file and focus the Vesta Editor window. The engine automatically compiles and reloads `GameModule.dll` while preserving serialized script state.

### 3. Exporting a Standalone Game
In the top menu bar, select **File > Export Game**. Vesta packages your compiled `GameModule.dll`, scenes, and assets (into `data.pak`) alongside `GameRuntime.exe` into your target output directory.

### 4. Running Tests
```powershell
cmake --build --preset debug --target QualTests
.\x64\Debug\QualTests.exe
```

### 5. Building Distribution Packages (Installer & Portable ZIP)
To generate a portable `.zip` archive and an Inno Setup installer (`Setup.exe`):
```powershell
.\scripts\package_release.ps1 -Version "0.2.0-alpha"
```
Output packages are written to the `dist/` directory.

---

## License

Licensed under the [MIT License](LICENSE).
