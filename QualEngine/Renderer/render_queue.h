#pragma once
#include <vector>
#include "render_item.h"

struct render_queue
{
    std::unordered_map<unsigned int, std::vector<render_item>> batches;
};