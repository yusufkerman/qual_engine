/**
 * @file vfs.cpp
 * @brief Sanal Dosya Sistemi ve QPAK paketleyici uygulamasini icerir.
 */

#include "vfs.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace qual
{
    vfs& vfs::instance()
    {
        static vfs s_instance;
        return s_instance;
    }

    vfs::~vfs()
    {
        unmount_all();
    }

    std::string vfs::normalize_path(const std::string& path)
    {
        std::string res = path;
        // Ters taksimleri ('\') duz taksime ('/') cevir
        std::replace(res.begin(), res.end(), '\\', '/');

        // Bastaki "./" veya "/" karakterlerini temizle
        while (res.rfind("./", 0) == 0)
        {
            res = res.substr(2);
        }
        while (!res.empty() && res.front() == '/')
        {
            res = res.substr(1);
        }

        // Kucuk/buyuk harf duyarsizligi icin kucuk harfe cevir (Windows dosya sistemi standardi)
        std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return res;
    }

    bool vfs::mount_pak(const std::string& pak_path)
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        std::ifstream file(pak_path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        qpak_header hdr{};
        file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        if (!file || hdr.magic != 0x4B415051)
        {
            std::cerr << "[VFS] Gecersiz QPAK basligi veya bozuk arsiv: " << pak_path << "\n";
            return false;
        }

        file.seekg(static_cast<std::streamoff>(hdr.index_offset), std::ios::beg);
        if (!file)
        {
            std::cerr << "[VFS] QPAK index tablosuna erisilemedi: " << pak_path << "\n";
            return false;
        }

        auto archive = std::make_unique<mounted_archive>();
        archive->file_path = pak_path;
        archive->header = hdr;

        for (uint32_t i = 0; i < hdr.entry_count; ++i)
        {
            uint16_t path_len = 0;
            file.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));

            std::string path_str(path_len, '\0');
            file.read(path_str.data(), path_len);

            vfs_file_entry entry{};
            entry.relative_path = path_str;
            file.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
            file.read(reinterpret_cast<char*>(&entry.size), sizeof(entry.size));
            file.read(reinterpret_cast<char*>(&entry.checksum), sizeof(entry.checksum));
            file.read(reinterpret_cast<char*>(&entry.flags), sizeof(entry.flags));

            const std::string norm = normalize_path(path_str);
            archive->entries[norm] = entry;
        }

        // Lookup haritasina ekle
        auto* raw_archive_ptr = archive.get();
        for (const auto& [norm_path, entry] : archive->entries)
        {
            m_file_lookup[norm_path] = { raw_archive_ptr, entry };
        }

        m_archives.push_back(std::move(archive));
        std::cout << "[VFS] Arsiv basariyla baglandi: " << pak_path << " (" << hdr.entry_count << " dosya)\n";
        return true;
    }

    void vfs::unmount_all()
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_file_lookup.clear();
        m_archives.clear();
    }

    bool vfs::is_mounted() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return !m_archives.empty();
    }

    bool vfs::has_file(const std::string& virtual_path) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        const std::string norm = normalize_path(virtual_path);
        return m_file_lookup.find(norm) != m_file_lookup.end();
    }

    std::vector<uint8_t> vfs::read_bytes(const std::string& virtual_path) const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        const std::string norm = normalize_path(virtual_path);
        auto it = m_file_lookup.find(norm);
        if (it == m_file_lookup.end())
        {
            return {};
        }

        const auto* archive = it->second.first;
        const auto& entry = it->second.second;

        std::ifstream file(archive->file_path, std::ios::binary);
        if (!file.is_open())
        {
            return {};
        }

        file.seekg(static_cast<std::streamoff>(entry.offset), std::ios::beg);
        std::vector<uint8_t> buffer(entry.size);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(entry.size));

        // Eger arsiv XOR ile sifrelenmisse coz
        if ((archive->header.flags & 0x01) != 0)
        {
            for (auto& byte : buffer)
            {
                byte ^= XOR_KEY;
            }
        }

        return buffer;
    }

    std::string vfs::read_text(const std::string& virtual_path) const
    {
        const auto bytes = read_bytes(virtual_path);
        if (bytes.empty())
        {
            return "";
        }
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    size_t vfs::get_file_count() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_file_lookup.size();
    }

    std::vector<std::string> vfs::get_all_file_paths() const
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        std::vector<std::string> paths;
        paths.reserve(m_file_lookup.size());
        for (const auto& [p, _] : m_file_lookup)
        {
            paths.push_back(p);
        }
        return paths;
    }

    bool vfs::create_pak_archive(
        const std::vector<std::string>& source_directories,
        const std::string& output_pak_path,
        bool obfuscate_xor)
    {
        namespace fs = std::filesystem;

        struct pending_file
        {
            std::string relative_path;
            std::string physical_path;
            uint64_t size;
        };

        std::vector<pending_file> files_to_pack;

        for (const auto& src_dir : source_directories)
        {
            if (!fs::exists(src_dir)) continue;

            const fs::path root(src_dir);
            for (const auto& entry : fs::recursive_directory_iterator(root))
            {
                if (entry.is_regular_file())
                {
                    const auto rel = fs::relative(entry.path(), root.parent_path());
                    std::string rel_str = rel.string();
                    std::replace(rel_str.begin(), rel_str.end(), '\\', '/');

                    pending_file pf;
                    pf.relative_path = rel_str;
                    pf.physical_path = entry.path().string();
                    pf.size = fs::file_size(entry.path());
                    files_to_pack.push_back(pf);
                }
            }
        }

        if (files_to_pack.empty())
        {
            std::cerr << "[VFS] Pakete eklenecek dosya bulunamadi.\n";
            return false;
        }

        fs::create_directories(fs::path(output_pak_path).parent_path());
        std::ofstream out(output_pak_path, std::ios::binary);
        if (!out.is_open())
        {
            std::cerr << "[VFS] Hedef .pak dosyasi yazilamadi: " << output_pak_path << "\n";
            return false;
        }

        qpak_header hdr{};
        hdr.magic = 0x4B415051;
        hdr.version = 1;
        hdr.flags = obfuscate_xor ? 0x01 : 0x00;
        hdr.entry_count = static_cast<uint32_t>(files_to_pack.size());
        hdr.index_offset = 0; // Dosya payloadi yazildiktan sonra guncellenecek

        // Gecici baslik yaz
        out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

        std::vector<vfs_file_entry> written_entries;
        written_entries.reserve(files_to_pack.size());

        // Payloadlari ard arda yaz
        for (const auto& pf : files_to_pack)
        {
            std::ifstream in(pf.physical_path, std::ios::binary);
            if (!in.is_open()) continue;

            vfs_file_entry entry{};
            entry.relative_path = pf.relative_path;
            entry.offset = static_cast<uint64_t>(out.tellp());
            entry.size = pf.size;

            std::vector<uint8_t> buffer(pf.size);
            in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(pf.size));

            uint32_t chk = 0;
            if (obfuscate_xor)
            {
                for (auto& b : buffer)
                {
                    chk += b;
                    b ^= XOR_KEY;
                }
            }
            else
            {
                for (auto b : buffer) chk += b;
            }
            entry.checksum = chk;

            out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            written_entries.push_back(entry);
        }

        // Index tablosunu dosya sonuna yaz
        const uint64_t index_offset = static_cast<uint64_t>(out.tellp());
        for (const auto& entry : written_entries)
        {
            const uint16_t path_len = static_cast<uint16_t>(entry.relative_path.size());
            out.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
            out.write(entry.relative_path.data(), path_len);

            out.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
            out.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
            out.write(reinterpret_cast<const char*>(&entry.checksum), sizeof(entry.checksum));
            out.write(reinterpret_cast<const char*>(&entry.flags), sizeof(entry.flags));
        }

        // Basligi guncelle
        hdr.index_offset = index_offset;
        hdr.entry_count = static_cast<uint32_t>(written_entries.size());
        out.seekp(0, std::ios::beg);
        out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        out.close();

        std::cout << "[VFS] data.pak paketi basariyla olusturuldu: " << output_pak_path
                  << " (" << written_entries.size() << " dosya, " << (index_offset / 1024) << " KB)\n";
        return true;
    }

} // namespace qual

