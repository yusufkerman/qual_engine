/**
 * @file gameplay_time.cpp
 * @brief gameplay::time statik fonksiyonlarının EngineContext köprüsü üzerinden uygulanması.
 */

#include "gameplay_time.h"
#include "gameplay_context.h"
#include "../Engine/engine_context.h"

namespace gameplay {

    float time::delta_time()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.delta_time() : 0.016f;
    }

    float time::unscaled_delta_time()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.unscaled_delta_time() : 0.016f;
    }

    float time::fixed_delta_time()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.fixed_delta_time() : (1.0f / 60.0f);
    }

    float time::total_time()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.total_time() : 0.0f;
    }

    float time::time_scale()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.time_scale() : 1.0f;
    }

    void time::set_time_scale(float scale)
    {
        auto* ctx = context::get_engine_context();
        if (ctx)
        {
            ctx->time.set_time_scale(scale);
        }
    }

    float time::fps()
    {
        auto* ctx = context::get_engine_context();
        return ctx ? ctx->time.fps() : 60.0f;
    }

} // namespace gameplay
