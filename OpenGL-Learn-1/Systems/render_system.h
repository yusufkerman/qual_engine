#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../Renderer/light_batch.h"
#include "../Engine/Assets/asset_manager.h"

class registry;
class MeshRenderer;
class shader_manager;
class material_manager;
struct render_item;

class render_system
{
public:
    void render(
        const registry& reg,
        const light_batch& lightBatch,
        MeshRenderer& renderer,
        shader_manager& shaderManager,
        asset_manager& assetManager,
        const glm::mat4& view,
        const glm::mat4& projection
    );

private:
    std::vector<render_item> extract(
        const registry& reg,
        asset_manager& assetManager,
        const glm::mat4& viewProj
    );
};