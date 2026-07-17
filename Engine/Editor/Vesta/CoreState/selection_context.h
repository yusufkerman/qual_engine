#pragma once

namespace vesta {

    enum class selection_context_type {
        scene_entities,  // Sahne Ağacı (Entity) seçimleri
        asset_files,     // Asset Browser (GUID / Path) seçimleri
        graph_nodes      // Node Editor (Material/Script) seçimleri
    };

} // namespace vesta