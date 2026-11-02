#pragma once

/**
 * @file vfs.h
 * @brief QualEngine Sanal Dosya Sistemi (VFS) ve QPAK binary arsiv yoneticisi.
 *
 * data.pak arsivlerinin bellege haritalanmasini (memory-mapping veya stream),
 * dosya iceriklerinin sifreli/sıkıstırılmıs paketlerden seffaf sekilde okunmasini
 * ve disk fallback mekanizmasini yonetir.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <optional>
#include <shared_mutex>

namespace qual
{
    /**
     * @brief QPAK Arsiv Baslik Yapisi (Header)
     */
    #pragma pack(push, 1)
    struct qpak_header
    {
        uint32_t magic = 0x4B415051;   ///< 'QPAK' (Little-endian: 0x51, 0x50, 0x41, 0x4B)
        uint32_t version = 1;          ///< QPAK v1
        uint32_t flags = 0;            ///< Bit 0: XOR Obfuscation (0x01)
        uint32_t entry_count = 0;      ///< Paketteki toplam dosya sayisi
        uint64_t index_offset = 0;     ///< Dizin giris tablosunun dosya ici baslangic ofseti
    };
    #pragma pack(pop)

    /**
     * @struct vfs_file_entry
     * @brief Sanal dosya dizininde bir dosyanin meta verisi.
     */
    struct vfs_file_entry
    {
        std::string relative_path; ///< "Assets/Textures/stone.png"
        uint64_t offset = 0;       ///< data.pak dosyasindaki veri ofseti
        uint64_t size = 0;         ///< Bayt cinsinden boyut
        uint32_t checksum = 0;     ///< Veri dogrulama checksum'i
        uint32_t flags = 0;
    };

    /**
     * @class vfs
     * @brief Sanal Dosya Sistemi tekil ornegi (Singleton).
     */
    class vfs
    {
    public:
        static vfs& instance();

        /**
         * @brief Bir .pak arsivini sisteme baglar (Mount).
         * @param pak_path Fiziksel diskteki .pak dosya yolu.
         * @return Basariyla yuklendiyse true.
         */
        bool mount_pak(const std::string& pak_path);

        /**
         * @brief Tum bagli arsivleri bellekten kaldirir (Unmount).
         */
        void unmount_all();

        /**
         * @brief Sisteme bagli aktif bir .pak arsivi olup olmadigini sorgular.
         */
        [[nodiscard]] bool is_mounted() const;

        /**
         * @brief Belirtilen sanal yola sahip bir dosyanin VFS'te bulunup bulunmadigini kontrol eder.
         */
        [[nodiscard]] bool has_file(const std::string& virtual_path) const;

        /**
         * @brief Dosyanin ham baytlarini (binary) okur. Bulunamazsa bos vektor doner.
         */
        [[nodiscard]] std::vector<uint8_t> read_bytes(const std::string& virtual_path) const;

        /**
         * @brief Dosyayi UTF-8 metin (text) olarak okur (Shader, JSON, txt vb.).
         */
        [[nodiscard]] std::string read_text(const std::string& virtual_path) const;

        /**
         * @brief Bagli paketteki toplam dosya sayisini doner.
         */
        [[nodiscard]] size_t get_file_count() const;

        /**
         * @brief VFS'teki tum dosya yollarinin listesini doner.
         */
        [[nodiscard]] std::vector<std::string> get_all_file_paths() const;

        /**
         * @brief Belirtilen dizin altindaki dosyalari tek bir .pak arsivine paketler (Export araci).
         * @param source_directories Pakete dahil edilecek klasorler (ornegin "Assets", "EngineResources").
         * @param output_pak_path Olusturulacak dosya yolu (ornegin "export/data.pak").
         * @param obfuscate_xor Icerigi gizlemek icin hafif XOR obfuskasyonu uygula.
         * @return Basariyla uretildiyse true.
         */
        static bool create_pak_archive(
            const std::vector<std::string>& source_directories,
            const std::string& output_pak_path,
            bool obfuscate_xor = true
        );

        /**
         * @brief Dosya yolunu VFS standart formatina (kucuk harf opsiyonu, duz taksim '/', baştaki './' temizleme) cevirir.
         */
        [[nodiscard]] static std::string normalize_path(const std::string& path);

    private:
        vfs() = default;
        ~vfs();
        vfs(const vfs&) = delete;
        vfs& operator=(const vfs&) = delete;

        struct mounted_archive
        {
            std::string file_path;
            qpak_header header;
            std::unordered_map<std::string, vfs_file_entry> entries;
        };

        mutable std::shared_mutex m_mutex;
        std::vector<std::unique_ptr<mounted_archive>> m_archives;
        std::unordered_map<std::string, std::pair<mounted_archive*, vfs_file_entry>> m_file_lookup;

        static constexpr uint8_t XOR_KEY = 0x5A;
    };

} // namespace qual

