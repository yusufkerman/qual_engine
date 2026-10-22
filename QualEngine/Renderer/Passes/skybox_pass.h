#pragma once
#include <glm/glm.hpp>

struct EngineContext;

namespace skybox_pass
{
    void render(EngineContext& ctx, const glm::mat4& view, const glm::mat4& projection);
}