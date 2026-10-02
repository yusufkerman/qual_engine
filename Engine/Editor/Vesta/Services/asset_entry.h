#pragma once

/**
 * @file asset_entry.h
 * @brief Varlık tarayıcısındaki (Asset Browser) dosya hiyerarşisi ve sürükle-bırak (Drag & Drop) işlemleri için veri yapılarını tanımlar.
 */

#include <string>
#include <vector>
#include <filesystem>
#include "../Engine/Assets/aria.h"

namespace vesta {

    /**
     * @enum asset_type
     * @brief Dosya sisteminde taranan varlığın temel türünü temsil eder.
     */
    enum class asset_type {
        folder,     ///< Klasör/Dizin.
        texture,    ///< Görüntü/Doku dosyası (.png, .jpg, vb.).
        model,      ///< 3D Model dosyası (.obj, .fbx, vb.).
        material,   ///< Materyal ayar dosyası (.json, .amat).
        unknown     ///< Desteklenmeyen veya bilinmeyen dosya türü.
    };

    /**
     * @struct asset_file_entry
     * @brief Varlık tarayıcısındaki (Asset Browser) ağaç görünümünde listelenecek tekil bir düğümü temsil eder.
     */
    struct asset_file_entry {
        std::string name;                       ///< Dosya veya klasörün adı (Uzantı dahil).
        std::filesystem::path full_path;        ///< Diskteki mutlak (absolute) yol.
        std::filesystem::path relative_path;    ///< Kök klasöre göre göreceli (relative) yol.
        asset_type type;                        ///< Dosyanın varlık türü.

        aria::asset_guid guid;                  ///< Motor tarafından dosyaya atanmış benzersiz kimlik (.meta).
        std::vector<asset_file_entry> children; ///< Eğer bu bir klasörse, içindeki alt dosyalar/klasörler.
    };

    /**
     * @enum asset_payload_type
     * @brief Editör içinde sürükle-bırak (Drag & Drop) işlemi sırasında taşınan verinin türünü belirler.
     */
    enum class asset_payload_type {
        texture,
        model,
        material
    };

    /**
     * @struct asset_drag_payload
     * @brief ImGui sürükle-bırak (Drag & Drop) mekanizması için tasarlanmış POD (Plain Old Data) veri paketi.
     * @note ImGui payload'ları 'trivially copyable' (bellekten doğrudan kopyalanabilir) olmalıdır.
     *       Bu nedenle std::string yerine sabit boyutlu char dizisi (MAX_PATH = 260) kullanılmıştır.
     */
    struct asset_drag_payload {
        asset_payload_type type;                ///< Taşınan varlığın türü.
        aria::asset_guid guid;                  ///< Taşınan varlığın GUID'si.
        char path[260];                         ///< Taşınan varlığın dosya yolu (Maksimum 260 karakter).
    };

} // namespace vesta