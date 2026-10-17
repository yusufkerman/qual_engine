/**
 * @file engine_resources.cpp
 * @brief Motor dahili kaynak yöneticisi uygulaması.
 */

#include "engine_resources.h"
#include "../Engine/Assets/aria.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace engine_resources {

    namespace {
        std::filesystem::path s_customPath;
        std::filesystem::path s_cachedResourcesPath;
        bool s_isPathCached = false;

        std::string to_lower_copy(std::string str) {
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return str;
        }

        std::string normalize_slashes(std::string path_str) {
            std::replace(path_str.begin(), path_str.end(), '\\', '/');
            return path_str;
        }

        std::filesystem::path get_executable_directory() {
            char buffer[MAX_PATH];
            DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
            if (length > 0) {
                return std::filesystem::path(buffer).parent_path();
            }
            return std::filesystem::current_path();
        }

        std::filesystem::path detect_engine_resources_path() {
            if (!s_customPath.empty() && std::filesystem::exists(s_customPath)) {
                return std::filesystem::canonical(s_customPath);
            }

            const auto exeDir = get_executable_directory();
            const auto cwd = std::filesystem::current_path();

            // Olası aday dizinler
            const std::filesystem::path candidates[] = {
                exeDir / "EngineResources",
                exeDir / "Resources",
                exeDir / ".." / ".." / "Engine" / "Resources",
                exeDir / ".." / "Engine" / "Resources",
                cwd / "Engine" / "Resources",
                cwd / ".." / "Engine" / "Resources",
                cwd / "EngineResources"
            };

            for (const auto& candidate : candidates) {
                std::error_code ec;
                if (std::filesystem::exists(candidate, ec) && std::filesystem::is_directory(candidate, ec)) {
                    return std::filesystem::canonical(candidate);
                }
            }

            // Fallback: Çalışma dizini altındaki Engine/Resources
            return (cwd / "Engine" / "Resources");
        }
    } // anonymous namespace

    const std::filesystem::path& get_resources_path() {
        if (!s_isPathCached) {
            s_cachedResourcesPath = detect_engine_resources_path();
            s_isPathCached = true;
            std::cout << "[EngineResources] Dahili kaynak kok dizini: " 
                      << s_cachedResourcesPath.string() << "\n";
        }
        return s_cachedResourcesPath;
    }

    void set_custom_resources_path(const std::filesystem::path& path) {
        s_customPath = path;
        s_isPathCached = false;
    }

    bool is_engine_resource_path(const std::string& path) {
        const std::string normalized = normalize_slashes(path);
        if (normalized.rfind("@engine/", 0) == 0 || normalized.rfind("engine://", 0) == 0) {
            return true;
        }

        const std::string lower = to_lower_copy(normalized);
        if (lower == "shaders/default_shader" ||
            lower == "shaders/skinned_default" ||
            lower == "shaders/debug/gizmo" ||
            lower == "shaders/debug/grid" ||
            lower == "shaders/skybox/skybox" ||
            lower == "shaders/shadow" ||
            lower == "shaders/shadow_skinned") {
            return true;
        }

        return false;
    }

    bool is_builtin_guid(const aria::asset_guid& guid) {
        return (guid == builtin_guid::default_shader ||
                guid == builtin_guid::skinned_default ||
                guid == builtin_guid::gizmo_shader ||
                guid == builtin_guid::grid_shader ||
                guid == builtin_guid::skybox_shader ||
                guid == builtin_guid::shadow_shader ||
                guid == builtin_guid::shadow_skinned_shader ||
                guid == builtin_guid::cube_mesh ||
                guid == builtin_guid::sphere_mesh);
    }

    std::optional<aria::asset_guid> get_builtin_guid_for_path(const std::string& path) {
        std::string n = normalize_slashes(path);

        // @engine/ veya engine:// öneklerini temizle
        if (n.rfind("@engine/", 0) == 0) {
            n = n.substr(8);
        } else if (n.rfind("engine://", 0) == 0) {
            n = n.substr(9);
        }

        const std::string lower = to_lower_copy(n);

        if (lower == "shaders/default_shader" || lower == "default_shader") return builtin_guid::default_shader;
        if (lower == "shaders/skinned_default" || lower == "skinned_default") return builtin_guid::skinned_default;
        if (lower == "shaders/debug/gizmo" || lower == "debug/gizmo" || lower == "gizmo") return builtin_guid::gizmo_shader;
        if (lower == "shaders/debug/grid" || lower == "debug/grid" || lower == "grid") return builtin_guid::grid_shader;
        if (lower == "shaders/skybox/skybox" || lower == "skybox/skybox" || lower == "skybox") return builtin_guid::skybox_shader;
        if (lower == "shaders/shadow" || lower == "shadow") return builtin_guid::shadow_shader;
        if (lower == "shaders/shadow_skinned" || lower == "shadow_skinned") return builtin_guid::shadow_skinned_shader;
        if (lower == "builtin/cube" || lower == "meshes/cube" || lower == "cube") return builtin_guid::cube_mesh;
        if (lower == "builtin/sphere" || lower == "meshes/sphere" || lower == "sphere") return builtin_guid::sphere_mesh;

        return std::nullopt;
    }

    std::string get_builtin_virtual_path_for_guid(const aria::asset_guid& guid) {
        if (guid == builtin_guid::default_shader) return "@engine/Shaders/default_shader";
        if (guid == builtin_guid::skinned_default) return "@engine/Shaders/skinned_default";
        if (guid == builtin_guid::gizmo_shader) return "@engine/Shaders/Debug/gizmo";
        if (guid == builtin_guid::grid_shader) return "@engine/Shaders/Debug/grid";
        if (guid == builtin_guid::skybox_shader) return "@engine/Shaders/Skybox/skybox";
        if (guid == builtin_guid::shadow_shader) return "@engine/Shaders/shadow";
        if (guid == builtin_guid::shadow_skinned_shader) return "@engine/Shaders/shadow_skinned";
        if (guid == builtin_guid::cube_mesh) return "engine://builtin/cube";
        if (guid == builtin_guid::sphere_mesh) return "engine://builtin/sphere";
        return "";
    }

    std::string resolve_path(const std::string& path) {
        if (path.empty()) return "";

        std::string n = normalize_slashes(path);

        // Sanal primitif mesh'ler disk dosyası değildir
        if (n == "engine://builtin/cube" || n == "engine://builtin/sphere") {
            return n;
        }

        const auto& resRoot = get_resources_path();

        // 1. "@engine/" öneki
        if (n.rfind("@engine/", 0) == 0) {
            std::string sub = n.substr(8);
            return (resRoot / sub).lexically_normal().string();
        }

        // 2. "engine://" öneki (primitif olmayan kaynaklar için)
        if (n.rfind("engine://", 0) == 0) {
            std::string sub = n.substr(9);
            return (resRoot / sub).lexically_normal().string();
        }

        // 3. Dahili shader göreli yolları (örn: "Shaders/default_shader")
        std::error_code ec;
        if (is_engine_resource_path(n)) {
            std::filesystem::path candidate = resRoot / n;
            // Eğer doğrudan uzantısız veya .vert/.frag olarak varsa resources dizininden dön
            if (std::filesystem::exists(candidate, ec) ||
                std::filesystem::exists(candidate.string() + ".vert", ec) ||
                std::filesystem::exists(candidate.string() + ".glsl", ec)) {
                return candidate.lexically_normal().string();
            }

            // Shaders/ öneki eksikse ekleyip dene
            std::filesystem::path shaderCandidate = resRoot / "Shaders" / n;
            if (std::filesystem::exists(shaderCandidate, ec) ||
                std::filesystem::exists(shaderCandidate.string() + ".vert", ec)) {
                return shaderCandidate.lexically_normal().string();
            }
        }

        // 4. Normal dosya yolu: Olduğu gibi dön
        return path;
    }

    void register_builtin_resources(aria::system* aria_sys) {
        if (!aria_sys) return;

        struct BuiltinEntry {
            aria::asset_guid guid;
            std::string virtual_path;
            std::string legacy_path;
            std::string disk_subpath;
        };

        const BuiltinEntry entries[] = {
            { builtin_guid::default_shader,        "@engine/Shaders/default_shader",        "Shaders/default_shader",        "Shaders/default_shader" },
            { builtin_guid::skinned_default,       "@engine/Shaders/skinned_default",       "Shaders/skinned_default",       "Shaders/skinned_default" },
            { builtin_guid::gizmo_shader,          "@engine/Shaders/Debug/gizmo",          "Shaders/Debug/gizmo",          "Shaders/Debug/gizmo" },
            { builtin_guid::grid_shader,           "@engine/Shaders/Debug/grid",           "Shaders/Debug/grid",           "Shaders/Debug/grid" },
            { builtin_guid::skybox_shader,         "@engine/Shaders/Skybox/skybox",        "Shaders/Skybox/skybox",        "Shaders/Skybox/skybox" },
            { builtin_guid::shadow_shader,         "@engine/Shaders/shadow",               "Shaders/shadow",               "Shaders/shadow" },
            { builtin_guid::shadow_skinned_shader, "@engine/Shaders/shadow_skinned",       "Shaders/shadow_skinned",       "Shaders/shadow_skinned" },
            { builtin_guid::cube_mesh,             "engine://builtin/cube",                "engine://builtin/cube",        "engine://builtin/cube" },
            { builtin_guid::sphere_mesh,           "engine://builtin/sphere",              "engine://builtin/sphere",      "engine://builtin/sphere" }
        };

        const auto& resRoot = get_resources_path();

        for (const auto& entry : entries) {
            std::string resolvedPath;
            if (entry.disk_subpath.rfind("engine://", 0) == 0) {
                resolvedPath = entry.disk_subpath;
            } else {
                resolvedPath = (resRoot / entry.disk_subpath).lexically_normal().string();
            }

            // Sabit GUID ile kayıt yap
            aria_sys->register_asset_path(entry.guid, resolvedPath);
        }

        std::cout << "[EngineResources] Motor dahili kaynaklari (Shaders & Meshes) Aria sistemine kaydedildi.\n";
    }

} // namespace engine_resources

