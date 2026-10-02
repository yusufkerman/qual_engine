#pragma once

/**
 * @file material_service.h
 * @brief Materyal varlıklarının (asset) oluşturulması, kopyalanması ve diske kaydedilmesi işlemlerini yöneten servis.
 */

#include "../Adapters/IEditableWorld.h"
#include "../CoreState/command_stack.h"
#include "../../../Engine/Assets/aria.h"
#include <string>
#include <filesystem>

namespace vesta {

    /**
     * @class material_service
     * @brief Materyal (material) dosyalarının disk üzerindeki yaşam döngüsünü ve motor içi (Aria) GUID senkronizasyonunu yönetir.
     */
    class material_service {
    private:
        IEditableWorld& m_world;
        command_stack& m_command_stack;

    public:
        /**
         * @brief Materyal servisini başlatır.
         *
         * @param world Sahneye ve ECS'ye erişim sağlayan dünya arayüzü.
         * @param cmd_stack Geri/İleri (Undo/Redo) işlemleri için komut yığını.
         */
        material_service(IEditableWorld& world, command_stack& cmd_stack);
        ~material_service() = default;

        /**
         * @brief Yeni bir materyal dosyası (.json) oluşturur ve Aria varlık sistemine (registry) kaydeder.
         *
         * @param directory_path Dosyanın oluşturulacağı hedef klasör.
         * @param name Oluşturulacak materyalin adı (Uzantısız).
         * @return aria::asset_guid Oluşturulan materyalin benzersiz kimliği. Başarısız olursa nil (boş) döner.
         */
        aria::asset_guid create_new_material(const std::filesystem::path& directory_path, const std::string& name = "new_material");

        /**
         * @brief Mevcut bir materyal dosyasını disk üzerinde çoğaltır (Duplicate) ve yeni bir varlık olarak kaydeder.
         *
         * @param source_guid Kopyalanacak kaynak materyalin GUID'si.
         * @param target_directory Kopyanın oluşturulacağı hedef klasör.
         * @return aria::asset_guid Kopyalanan yeni materyalin GUID'si. Başarısız olursa nil döner.
         */
        aria::asset_guid duplicate_material(const aria::asset_guid& source_guid, const std::filesystem::path& target_directory);

        /**
         * @brief Değiştirilmiş materyal verisini (JSON) doğrudan diske kaydeder.
         *
         * @param guid Kaydedilecek materyalin benzersiz kimliği.
         * @param serialized_data Diske yazılacak olan serileştirilmiş metin verisi.
         * @return bool İşlem başarılıysa true, aksi halde false.
         */
        bool save_material_changes(const aria::asset_guid& guid, const std::string& serialized_data);
    };

} // namespace vesta