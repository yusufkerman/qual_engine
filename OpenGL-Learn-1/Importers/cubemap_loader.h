#pragma once

#include <string>
#include <vector>

namespace cubemap_loader
{
    // 1) KLASSİK: 6 ayrı yüz
    unsigned int load_faces(const std::vector<std::string>& faces);

    // 2) ATLAS / TEK PNG (6 yüz tek image içinde)
    unsigned int load_atlas(const std::string& path);
}