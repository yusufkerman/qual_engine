#pragma once

/**
 * @file asset_database.h
 * @brief Varlıkların (asset) UUID değerleri ile fiziksel dosya yolları arasındaki ilişkiyi kuran ve yöneten veritabanı sınıfı.
 */

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <functional> 
#include "../Externals/uuid/uuid.h"
#include "../Externals/nlohmann/json.hpp"

namespace aria {

    /**
     * @struct asset_guid_hash
     * @brief stduuid kütüphanesindeki UUID'ler için std::unordered_map uyumlu hash uzmanlaşması (specialization) sağlar.
     */
    struct asset_guid_hash {
        std::size_t operator()(const uuids::uuid& guid) const noexcept {
            return std::hash<uuids::uuid>{}(guid);
        }
    };

    /**
     * @class asset_database
     * @brief GUID tabanlı varlık adres defterini bellekte tutar ve diske (JSON) serileştirme/deserileştirme işlemlerini yönetir.
     */
    class asset_database {
    private:
        /// GUID'leri disk üzerindeki fiziksel dosya yollarına bağlayan hash tablosu.
        std::unordered_map<uuids::uuid, std::string, asset_guid_hash> m_guid_to_path;

    public:
        asset_database() = default;

        /**
         * @brief Yeni bir varlığı GUID ve dosya yolu eşleşmesiyle adres defterine kaydeder.
         *
         * @param guid Kaydedilecek varlığın benzersiz kimliği.
         * @param path Varlığın disk üzerindeki fiziksel yolu.
         */
        void register_asset(const uuids::uuid& guid, const std::string& path) {
            if (guid.is_nil()) return;
            m_guid_to_path[guid] = path;
        }

        /**
         * @brief Verilen GUID'ye karşılık gelen fiziksel dosya yolunu bulur.
         *
         * @param guid Sorgulanacak varlığın benzersiz kimliği.
         * @return std::string Varlığın dosya yolu. GUID bulunamazsa boş string ("") döner.
         */
        std::string resolve_path(const uuids::uuid& guid) const {
            auto it = m_guid_to_path.find(guid);
            if (it != m_guid_to_path.end()) {
                return it->second;
            }
            return "";
        }

        /**
         * @brief Bellekteki adres defterini (manifest) JSON formatında diske kaydeder.
         *
         * @param manifest_path Kaydedilecek JSON dosyasının hedef yolu (örn: 'asset_manifest.json').
         * @return bool Kayıt işlemi başarılı olursa true, hata oluşursa false döner.
         */
        bool save_to_disk(const std::string& manifest_path) const {
            try {
                nlohmann::json manifest_json = nlohmann::json::object();
                for (const auto& [guid, path] : m_guid_to_path) {
                    manifest_json[uuids::to_string(guid)] = path;
                }
                std::ofstream file(manifest_path);
                if (!file.is_open()) return false;
                file << manifest_json.dump(4);
                return true;
            }
            catch (...) {
                return false;
            }
        }

        /**
         * @brief Diskteki JSON formatlı adres defterini (manifest) okur ve belleğe yükler.
         *
         * @param manifest_path Okunacak JSON dosyasının yolu.
         * @return bool Yükleme ve ayrıştırma işlemi başarılı olursa true, hata oluşursa false döner.
         */
        bool load_from_disk(const std::string& manifest_path) {
            try {
                std::ifstream file(manifest_path);
                if (!file.is_open()) return false;

                nlohmann::json manifest_json;
                file >> manifest_json;

                m_guid_to_path.clear();
                for (auto& [guid_str, path_val] : manifest_json.items()) {
                    auto parsed = uuids::uuid::from_string(guid_str);
                    if (parsed) {
                        m_guid_to_path[*parsed] = path_val.get<std::string>();
                    }
                }
                return true;
            }
            catch (...) {
                return false;
            }
        }

        /**
         * @brief Bellekteki adres defteri tablosunu tamamen temizler.
         */
        void clear() {
            m_guid_to_path.clear();
        }
    };

} // namespace aria