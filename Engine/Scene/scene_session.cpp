/**
 * @file scene_session.cpp
 * @brief Tek canlı sahne (Single Live World) oynatma/durdurma yaşam döngüsü ve snapshot geri yükleme uygulaması.
 */

#include "scene_session.h"
#include "../Engine/engine_context.h"
#include <utility>

scene& scene_session::editor_scene() noexcept
{
    return m_liveScene;
}

const scene& scene_session::editor_scene() const noexcept
{
    return m_liveScene;
}

scene& scene_session::game_scene() noexcept
{
    return m_liveScene;
}

const scene& scene_session::game_scene() const noexcept
{
    return m_liveScene;
}

bool scene_session::is_playing() const noexcept
{
    return m_playing;
}

void scene_session::play(EngineContext& ctx, bool takeSnapshot)
{
    if (m_playing)
    {
        return;
    }

    // Tek seferlik yedek: STOP'ta sahnenin PLAY öncesi haline dönülebilmesi için.
    // Bu kopya yalnızca mod geçişinde bir kez yapılır; kareler arası hiçbir senkronizasyon maliyeti yoktur.
    m_hasSnapshot = takeSnapshot;
    if (m_hasSnapshot)
    {
        m_playSnapshot = clone_scene(m_liveScene);
    }

    m_playing = true;
    m_liveScene.on_runtime_start(ctx);
}

void scene_session::stop()
{
    if (!m_playing)
    {
        return;
    }

    m_liveScene.clean_up_scripts();

    if (m_hasSnapshot)
    {
        // Move ataması: registry nesnesinin adresi değişmez, editör panellerinin tuttuğu referanslar geçerli kalır.
        m_liveScene = std::move(m_playSnapshot);
        m_playSnapshot = scene{};
        m_hasSnapshot = false;
    }

    m_playing = false;
}

scene scene_session::clone_scene(const scene& src)
{
    scene newScene;

    const registry& srcReg = src.get_registry();
    registry& dstReg = newScene.get_registry();

    const auto& srcEntities = srcReg.get_all_entities();

    std::vector<entity> clonedEntities;
    clonedEntities.reserve(srcEntities.size());

    for (const entity e : srcEntities)
    {
        srcReg.clone_entity(e, dstReg);
        clonedEntities.push_back(e);
    }

    newScene.set_entities(clonedEntities);
    newScene.set_active_camera(src.get_active_camera());

    return newScene;
}