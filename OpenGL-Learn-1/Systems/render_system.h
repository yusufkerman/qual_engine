#pragma once

#include "../Shader.h"
#include <glm/glm.hpp>

class registry;
class MeshRenderer;

class render_system
{
public:
    void render(
        const registry& reg,
        MeshRenderer& renderer,
        Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection
    );
};