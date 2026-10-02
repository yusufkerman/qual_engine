#pragma once

/**
 * @file selection_context.h
 * @brief Editör genelinde anlık olarak odaklanılan seçim bağlamı türlerini tanımlar.
 */

namespace vesta {

    /**
     * @enum selection_context_type
     * @brief Editör arayüzünde o an hangi panelin veya veri havuzunun aktif seçime sahip olduğunu belirtir.
     */
    enum class selection_context_type {
        scene_entities,  ///< Sahne hiyerarşisindeki nesne (Entity) seçimleri.
        asset_files,     ///< Varlık tarayıcısındaki (Asset Browser) dosya ve GUID seçimleri.
        graph_nodes      ///< Graf editörlerindeki (Material/Script Graph) düğüm seçimleri.
    };

} // namespace vesta