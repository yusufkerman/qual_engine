#pragma once

#include "../../Scene/scene.h"

struct EngineContext;
struct EditorContext;

class inspector_panel
{
public:
    void render(scene& scene,
        int selectedEntity, 
        EngineContext& engineCtx,
        EditorContext& editorCtx);
};