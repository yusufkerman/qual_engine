/**
 * @file runtime_layer.cpp
 * @brief ECS-Fizik senkronizasyonu, sabit zaman adımlı simülasyon ve sahne veri aktarımı uygulaması.
 */

#include "runtime_layer.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "../Systems/physics_bridge.h"

namespace {

    constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f; ///< 60 Hz sabit fizik simülasyon zaman adımı.

    /**
     * @brief Aktif oyun kamerasının çalışma zamanı hareket veya takip mantığını günceller.
     */
    void update_runtime_camera(EngineContext& ctx)
    {
        scene& gameScene = ctx.session.game_scene();
        const entity activeCam = gameScene.get_active_camera();

        if (activeCam == 0) {
            return;
        }

        auto* camT = gameScene.get_registry().get_component<transform_component>(activeCam);
        if (!camT) {
            return;
        }

        // Calisma zamani kamera davranisi (script / kontrolcu) buraya eklenecektir.
    }

} // anonymous namespace

void runtime_layer::update(EngineContext& ctx, EditorContext& editorCtx)
{
    // Yalnızca oyun oynatma modundayken simülasyonu yürüt
    if (editorCtx.mode != EngineMode::PLAY) {
        return;
    }

    // 1. ECS bileşen verilerinde (Inspector veya Sahne Gizmosu ile) yapılan anlık değişiklikleri fizik motoruna aktar
    physics_bridge::sync_ecs_to_physics(ctx, ctx.session.game_scene());

    // Oyun duraklatılmışsa (Paused) fizik ve kamera ilerletmesini atla
    if (editorCtx.isPaused) {
        return;
    }

    // 2. Kamera mantığını işlet
    update_runtime_camera(ctx);

    // 3. Fizik motorunu sabit adımla ilerlet
    ctx.physicsSystem.update(FIXED_DELTA_TIME);

    // 4. Simülasyon sonuçlarını (konum/rotasyon) tekrar canlı ECS bileşenlerine yaz (Zero-Copy: kopyalama yapılmaz)
    physics_bridge::sync_physics_to_ecs(ctx, ctx.session.game_scene());
}