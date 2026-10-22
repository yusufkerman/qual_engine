#pragma once

#include "engine_context.h"
#include "../../Engine/Editor/editor_context.h"

namespace Engine
{
    bool init(EngineContext& ctx);
    void update(EngineContext& ctx, EditorContext& editorCtx);
    void render(EngineContext& ctx);
    void shutdown(EngineContext& ctx);
}