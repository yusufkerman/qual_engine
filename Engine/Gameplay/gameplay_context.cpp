/**
 * @file gameplay_context.cpp
 * @brief Gameplay API bağlam köprüsü uygulaması.
 */

#include "gameplay_context.h"
#include "../Engine/engine_context.h"
#include "../Scene/scene.h"

namespace gameplay
{
    void context::set_engine_context(EngineContext* ctx)
    {
        s_engine_ctx = ctx;
    }

    void context::set_active_scene(scene* s)
    {
        s_active_scene = s;
    }

    void context::set_collision_queue(collision_event_queue* q)
    {
        s_collision_queue = q;
    }

    bool context::is_initialized()
    {
        return s_engine_ctx != nullptr;
    }

    bool context::is_editor_build()
    {
        return s_engine_ctx ? s_engine_ctx->isEditorBuild : false;
    }

    EngineContext* context::get_engine_context()
    {
        return s_engine_ctx;
    }

    registry* context::get_registry()
    {
        if (s_engine_ctx)
        {
            return &s_engine_ctx->session.game_scene().get_registry();
        }
        if (s_active_scene)
        {
            return &s_active_scene->get_registry();
        }
        return nullptr;
    }

    physics_system* context::get_physics_system()
    {
        if (s_engine_ctx)
        {
            return &s_engine_ctx->physicsSystem;
        }
        return nullptr;
    }

    InputState* context::get_input_state()
    {
        if (s_engine_ctx)
        {
            return &s_engine_ctx->inputState;
        }
        return nullptr;
    }

    collision_event_queue* context::get_collision_queue()
    {
        return s_collision_queue;
    }

    scene* context::get_active_scene()
    {
        if (s_engine_ctx)
        {
            return &s_engine_ctx->session.game_scene();
        }
        if (s_active_scene)
        {
            return s_active_scene;
        }
        return nullptr;
    }
}

