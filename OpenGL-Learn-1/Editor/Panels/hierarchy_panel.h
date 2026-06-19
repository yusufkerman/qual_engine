#pragma once
#include "../../Scene/scene.h"

class hierarchy_panel
{

public:
    void render(scene& scene,
        int& selectedEntity,
        int& renamingEntity,
        char* renameBuffer,
        std::vector<entity>& deleteQueue);
};