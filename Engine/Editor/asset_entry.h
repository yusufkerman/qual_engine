#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem>

#include "../Engine/Assets/aria.h"

namespace aria::editor {

    enum class asset_type {
        folder,
        texture,
        model,
        material,
        unknown
    };

    struct asset_file_entry {
        std::string name;
        std::filesystem::path full_path;
        std::filesystem::path relative_path;
        asset_type type;

        aria::asset_guid guid;
        std::vector<asset_file_entry> children;
    };

    enum class asset_payload_type {
        texture,
        model,
        material
    };

    // ❗ FIX: duplicate type + net payload
    struct asset_drag_payload {
        asset_payload_type type;
        aria::asset_guid guid;
        char path[260];
    };

} // namespace aria::editor