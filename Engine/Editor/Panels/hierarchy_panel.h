#pragma once

#include "../../Scene/scene.h"
#include "../editor_context.h"
#include <vector>


struct hierarchy_panel
{
    void render(scene& scene,
        int& selectedEntity,
        int& renamingEntity,
        char* renameBuffer,
        size_t renameBufferSize,   
        std::vector<entity>& deleteQueue,
        EditorContext& ctx);
};