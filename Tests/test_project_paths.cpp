#include <doctest/doctest.h>
#include "../Engine/Core/path_utils.h"
#include "../Engine/Project/engine_settings.h"
#include <filesystem>
#include <fstream>
#include <string>

TEST_SUITE("ProjectAndPathUtils")
{
    TEST_CASE("PathUtils: Turkish & Non-ASCII UTF-8 Conversion Integrity")
    {
        const std::string originalUtf8 = "Yazılım Projeleri/QualEngine Örnek Oyun/Şafak Çeşmesi ğüşıöç";
        std::filesystem::path p = qual::path_from_utf8(originalUtf8);
        std::string roundtrip = qual::path_to_utf8(p);

        // Normalize slashes for comparison
        std::string n1 = originalUtf8;
        std::string n2 = roundtrip;
        std::replace(n1.begin(), n1.end(), '\\', '/');
        std::replace(n2.begin(), n2.end(), '\\', '/');

        CHECK(n1 == n2);
    }

    TEST_CASE("PathUtils: File I/O in Non-ASCII Folder")
    {
        namespace fs = std::filesystem;
        const std::string testFolderUtf8 = "qual_test_yazilim_projeleri_çşğüöı";
        const fs::path dir = fs::temp_directory_path() / qual::path_from_utf8(testFolderUtf8);

        std::error_code ec;
        fs::create_directories(dir, ec);
        CHECK(!ec);

        const fs::path file = dir / qual::path_from_utf8("başlangıç_sahnesi.json");
        const std::string testContent = "{\"scene_name\": \"Türkçe Sahne Testi\", \"camera\": true}";

        {
            std::ofstream out(file);
            CHECK(out.is_open());
            out << testContent;
            out.close();
        }

        CHECK(fs::exists(file));

        {
            std::ifstream in(file);
            CHECK(in.is_open());
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            CHECK(content == testContent);
        }

        fs::remove_all(dir, ec);
    }

    TEST_CASE("EngineSettings: Last Project Path Persistence")
    {
        auto& settings = project::engine_settings::instance();
        const std::string testProjPath = "C:/Test/Yazılım Projeleri/Test1/project.json";

        settings.set_last_project_path(testProjPath);
        CHECK(settings.get_last_project_path() == testProjPath);
    }
}

