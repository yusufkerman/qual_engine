#pragma once

#include <string>
#include <vector>
#include <cstdint>
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
}