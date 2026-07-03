#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <functional> // std::hash için gerekli
#include "../Externals/uuid/uuid.h"      // Projendeki stduuid yolu
#include "../Externals/nlohmann/json.hpp" // Projendeki json yolu

namespace aria {

    // 🚀 DÜZELTME: stduuid'in standart std::hash uzmanlaşmasını tetikliyoruz
    struct asset_guid_hash {
        std::size_t operator()(const uuids::uuid& guid) const noexcept {
            return std::hash<uuids::uuid>{}(guid);
        }
    };

    class asset_database {
    private:
        // GUID -> Disk üzerindeki gerçek dosya yolu (Path) eşleşme tablosu
        std::unordered_map<uuids::uuid, std::string, asset_guid_hash> m_guid_to_path;

    public:
        asset_database() = default;

        // 📝 Yeni bir kaynağı adres defterine kaydeder
        void register_asset(const uuids::uuid& guid, const std::string& path) {
            if (guid.is_nil()) return;
            m_guid_to_path[guid] = path;
        }

        // 🔍 Elimizdeki GUID'in diskte nerede olduğunu bulur
        std::string resolve_path(const uuids::uuid& guid) const {
            auto it = m_guid_to_path.find(guid);
            if (it != m_guid_to_path.end()) {
                return it->second;
            }
            return ""; // Bulunamazsa boş döner
        }

        // 💽 Tüm adres defterini diske 'asset_manifest.json' olarak kaydeder
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
            catch (...) { return false; }
        }

        // 📂 Diskteki manifest dosyasını belleğe geri yükler
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
            catch (...) { return false; }
        }

        void clear() { m_guid_to_path.clear(); }
    };

} // namespace aria