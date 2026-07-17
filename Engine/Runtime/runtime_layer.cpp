#include "runtime_layer.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "../Systems/physics_bridge.h"

static void update_runtime_camera(EngineContext& ctx)
{
    scene& gameScene = ctx.session.game_scene();

    entity activeCam = gameScene.get_active_camera();

    auto* camT =
        (activeCam != 0)
        ? gameScene.get_registry().get_component<transform_component>(activeCam)
        : nullptr;

    if (!camT)
        return;

    
}

void runtime_layer::update(EngineContext& ctx, EditorContext& editorCtx)
{
    if (editorCtx.mode != EngineMode::PLAY)
        return;

    update_runtime_camera(ctx);

    physics_bridge::sync_ecs_to_physics(ctx, ctx.session.game_scene());

    ctx.physicsSystem.update(0.016f);

    physics_bridge::sync_physics_to_ecs(ctx, ctx.session.game_scene());

    ctx.session.sync_editor_from_runtime();
}