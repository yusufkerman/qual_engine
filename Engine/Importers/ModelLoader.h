#pragma once

#include "../Graphics/model.h"
#include <string>

class ModelLoader
{
public:
    static Model Load(const std::string& path);
};