#include "scene_session.h"

scene& scene_session::editor_scene()
{
    return editorScene;
}

const scene& scene_session::editor_scene() const
{
    return editorScene;
}

scene& scene_session::runtime_scene()
{
    return runtimeScene;
}

const scene& scene_session::runtime_scene() const
{
    return runtimeScene;
}

scene& scene_session::game_scene()
{
    return playing ? runtimeScene : editorScene;
}

const scene& scene_session::game_scene() const
{
    return playing ? runtimeScene : editorScene;
}

bool scene_session::is_playing() const
{
    return playing;
}

void scene_session::play()
{
    if (playing)
        return;

    editorSnapshot = clone_scene(editorScene);
    runtimeScene = clone_scene(editorScene);
    playing = true;
}

void scene_session::stop()
{
    if (!playing)
        return;

    editorScene = clone_scene(editorSnapshot);
    runtimeScene = scene();
    playing = false;
}

void scene_session::sync_editor_from_runtime()
{
    if (!playing)
        return;

    editorScene = clone_scene(runtimeScene);
}

scene scene_session::clone_scene(const scene& src)
{
    scene newScene;

    const registry& srcReg = src.get_registry();
    registry& dstReg = newScene.get_registry();

    newScene.set_entities(src.get_entities());

    for (auto e : srcReg.get_all_entities())
    {
        dstReg.add_entity(e);

        if (srcReg.has_transform(e))
            dstReg.add_transform(e, *srcReg.get_transform(e));

        if (srcReg.has_mesh(e))
            dstReg.add_mesh(e, *srcReg.get_mesh(e));

        if (srcReg.has_camera(e))
            dstReg.add_camera(e, *srcReg.get_camera(e));

        if (srcReg.has_light(e))
            dstReg.add_light(e, *srcReg.get_light(e));
    }

    newScene.set_active_camera(src.get_active_camera());

    return newScene;
}