#pragma once

#include "engine_context.h"

namespace Engine
{
    bool init(EngineContext& ctx, EditorContext& editorCtx);
    void update(EngineContext& ctx, EditorContext& editorCtx);
    void render(EngineContext& ctx, EditorContext& editorCtx);
    void shutdown(EngineContext& ctx, EditorContext& editorCtx);
}