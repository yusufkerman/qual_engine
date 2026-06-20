#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../Renderer/render_item.h"

class registry;
class MeshRenderer;
class shader_manager;
class material_manager;

class render_system
{
public:
    void render(
        const registry& reg,
        MeshRenderer& renderer,
        shader_manager& shaderManager,
        const material_manager& materialManager,
        const glm::mat4& view,
        const glm::mat4& projection
    );

    std::vector<render_item> extract(
        const registry& reg,
        const material_manager& materialManager,
        const glm::mat4& viewProj
    );
};