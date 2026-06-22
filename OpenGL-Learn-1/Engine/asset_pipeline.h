#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <glm/glm.hpp>

class EngineContext;

using asset_id = uint32_t;

struct imported_model_result
{
    std::vector<asset_id> mesh_ids;
    glm::vec3 boundsCenter{ 0.0f };
    float boundsRadius = 0.0f;
};

namespace asset_pipeline
{
    imported_model_result import_model(EngineContext& ctx, const std::string& path);

    asset_id import_texture(EngineContext& ctx, const std::string& path);

    // --- YENİ PRİMİTİVE FONKSİYONLARI ---
    // 1x1x1 boyutlarında statik bir küp üretip GPU / AssetManager'a yükler
    imported_model_result create_cube_mesh(EngineContext& ctx);

    // Belirtilen yarıçapa sahip bir küre üretip GPU / AssetManager'a yükler
    imported_model_result create_sphere_mesh(EngineContext& ctx, float radius = 1.0f);

    void clear_cache();


}