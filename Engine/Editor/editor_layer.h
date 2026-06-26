#pragma once

struct EngineContext;
struct EditorContext;

class editor_layer
{
public:
    static void update(EngineContext& ctx, EditorContext& editorCtx);
    static void render(EngineContext& ctx, EditorContext& editorCtx);
};