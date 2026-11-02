#include <doctest/doctest.h>
#include "../Engine/Core/VFS/vfs.h"
#include <filesystem>
#include <fstream>
#include <string>

TEST_SUITE("VirtualFileSystem")
{
    TEST_CASE("VFS: Path Normalization")
    {
        CHECK(qual::vfs::normalize_path("Assets\\Shaders\\default.vert") == "assets/shaders/default.vert");
        CHECK(qual::vfs::normalize_path("./Assets/Textures/stone.png") == "assets/textures/stone.png");
        CHECK(qual::vfs::normalize_path("/EngineResources/skybox.frag") == "engineresources/skybox.frag");
        CHECK(qual::vfs::normalize_path(".\\Data\\Model.OBJ") == "data/model.obj");
    }

    TEST_CASE("VFS: QPAK Archive Creation, Mounting & Transparent Read")
    {
        namespace fs = std::filesystem;
        const fs::path test_dir = fs::temp_directory_path() / "qual_vfs_test";
        const fs::path content_dir = test_dir / "Content";
        const fs::path pak_file = test_dir / "test_data.pak";

        fs::create_directories(content_dir / "Shaders");
        fs::create_directories(content_dir / "Texts");

        // 1. Create dummy files
        const std::string shader_content = "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }";
        const std::string text_content = "QualEngine VFS QPAK Test Content 2026";
        const std::vector<uint8_t> binary_content = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04 };

        {
            std::ofstream s(content_dir / "Shaders" / "test.vert", std::ios::binary);
            s << shader_content;
        }
        {
            std::ofstream t(content_dir / "Texts" / "hello.txt");
            t << text_content;
        }
        {
            std::ofstream b(content_dir / "binary.bin", std::ios::binary);
            b.write(reinterpret_cast<const char*>(binary_content.data()), binary_content.size());
        }

        // 2. Create QPAK Archive with XOR obfuscation
        const bool created = qual::vfs::create_pak_archive({ content_dir.string() }, pak_file.string(), true);
        CHECK(created);
        CHECK(fs::exists(pak_file));
        CHECK(fs::file_size(pak_file) > 0);

        // 3. Mount Archive in VFS
        qual::vfs& vfs = qual::vfs::instance();
        vfs.unmount_all();
        CHECK(vfs.is_mounted() == false);

        const bool mounted = vfs.mount_pak(pak_file.string());
        CHECK(mounted);
        CHECK(vfs.is_mounted() == true);
        CHECK(vfs.get_file_count() >= 3);

        // 4. Test File Lookups and Content Extraction
        CHECK(vfs.has_file("Content/Shaders/test.vert") == true);
        CHECK(vfs.has_file("Content\\Texts\\hello.txt") == true);
        CHECK(vfs.has_file("content/binary.bin") == true);
        CHECK(vfs.has_file("non_existent_file.dat") == false);

        const std::string read_shader = vfs.read_text("Content/Shaders/test.vert");
        CHECK(read_shader == shader_content);

        const std::string read_txt = vfs.read_text("Content/Texts/hello.txt");
        CHECK(read_txt == text_content);

        const std::vector<uint8_t> read_bin = vfs.read_bytes("Content/binary.bin");
        CHECK(read_bin == binary_content);

        // 5. Unmount and Clean
        vfs.unmount_all();
        CHECK(vfs.is_mounted() == false);
        CHECK(vfs.has_file("Content/Texts/hello.txt") == false);

        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }
}
