#pragma once
#include <string>

struct texture_asset {
    unsigned int gl_id = 0;       // VRAM'deki OpenGL ID'si (Handle)
    std::string path = "None";    // Disk yolu
};