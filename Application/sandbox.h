#pragma once

struct EngineContext;
struct EditorContext;


void sandbox_load_default_scene(EngineContext& ctx);

void sandbox_load_scene2_directly(EngineContext& ctx);

void sandbox_load_demo_game(EngineContext& ctx, EditorContext& edtitorCtx);