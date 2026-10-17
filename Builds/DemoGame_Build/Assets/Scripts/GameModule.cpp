/**
 * @file GameModule.cpp
 * @brief DemoGame projesi dinamik oyun modülü dışa aktarma noktası.
 */

#include <Engine/Scripting/game_module.h>
#include "player_controller.h"
#include "camera_controller.h"

GAME_MODULE_BEGIN()
    GAME_MODULE_SCRIPT(player_controller)
    GAME_MODULE_SCRIPT(camera_controller)
GAME_MODULE_END()

