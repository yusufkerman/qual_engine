#pragma once

class scene;

#include "../ECS/entity.h"

class camera_system
{
public:
    void update(scene& scn);

private:
    entity m_lastCamera = 0;
};