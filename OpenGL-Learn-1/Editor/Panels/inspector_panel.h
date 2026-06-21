#pragma once

#include "../../Scene/scene.h"

class EngineContext;

class inspector_panel
{
public:
    void render(scene& scene,
        int selectedEntity, 
        EngineContext& ctx);
};