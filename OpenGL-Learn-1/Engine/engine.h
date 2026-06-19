#pragma once

#include "engine_context.h"

namespace Engine
{
    bool init(EngineContext& ctx);
    void update(EngineContext& ctx);
    void render(EngineContext& ctx);
    void shutdown(EngineContext& ctx);
}