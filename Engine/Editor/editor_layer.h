#pragma once

struct EngineContext;

class editor_layer
{
public:
    static void update(EngineContext& ctx);
    static void render(EngineContext& ctx);
};