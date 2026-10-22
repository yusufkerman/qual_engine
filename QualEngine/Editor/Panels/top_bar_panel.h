#pragma once
#include "../engine_mode.h"

class scene;
struct EngineContext;

class top_bar_panel
{
public:
    // Parametreleri güncelledik
    void render(EngineMode& mode, scene& currentScene, EngineContext& ctx);
};