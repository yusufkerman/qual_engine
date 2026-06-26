#pragma once

#include "scene.h"
#include "../Systems/camera_system.h"

class scene_session {
public:
    scene& editor_scene();
    const scene& editor_scene() const;

    scene& runtime_scene();
    const scene& runtime_scene() const;

    scene& game_scene();
    const scene& game_scene() const;

    bool is_playing() const;

    void play(EngineContext& ctx);
    void stop();

    void sync_editor_from_runtime();

private:
    scene editorScene;
    scene runtimeScene;
    scene editorSnapshot;
    bool playing = false;

private:
    scene clone_scene(const scene& src);
};