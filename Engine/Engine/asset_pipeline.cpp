#include "asset_pipeline.h"
#include "engine_context.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../Externals/uuid/uuid.h" 
#include "../Externals/nlohmann/json.hpp" // JSON kütüphanen

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace asset_pipeline
{
    // Helper: Meta dosyasından UUID okur veya yoksa yenisini üretip kaydeder
    aria::asset_guid get_or_create_meta_guid(const std::string& asset_path)
    {
        std::string meta_path = asset_path + ".meta";

        // 1. Eğer .meta dosyası zaten varsa içindeki UUID'yi oku
        if (fs::exists(meta_path)) {
            try {
                std::ifstream file(meta_path);
                json meta_json;
                file >> meta_json;
                file.close();

                std::string guid_str = meta_json.value("guid", "");
                auto parsed_guid = uuids::uuid::from_string(guid_str);
                if (parsed_guid) {
                    return *parsed_guid; // Geçerli meta UUID'sini döndür
                }
            }
            catch (...) {
                std::cout << "[AssetPipeline] Uyari: Meta dosyasi okunurken hata olustu, yeniden uretilecek: " << meta_path << "\n";
            }
        }

        // 2. Meta dosyası yoksa veya bozuksa: SIFIRDAN UUID v4 ÜRET
        std::random_device rd;
        auto seed_data = std::array<int, std::numeric_limits<int>::digits>{};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 engine(seq);
        uuids::uuid_random_generator gen(engine);
        aria::asset_guid yeni_guid = gen();

        // 3. Yeni üretilen UUID'yi .meta dosyası olarak diske kilitle
        try {
            json meta_json;
            meta_json["guid"] = uuids::to_string(yeni_guid); // stduuid string serialization

            std::ofstream file(meta_path);
            file << meta_json.dump(4); // Okunabilir şekilde 4 tab boşlukla yaz
            file.close();
            std::cout << "[AssetPipeline] Yeni .meta dosyasi olusturuldu: " << meta_path << "\n";
        }
        catch (...) {
            std::cout << "[AssetPipeline] Hata: .meta dosyasi diske yazilamadi!\n";
        }

        return yeni_guid;
    }

    aria::handle<aria::model_asset> import_model(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);

        return ctx.aria_system->import_model(guid, path);
    }

    aria::handle<aria::texture_asset> import_texture(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);

        return ctx.aria_system->import_texture(guid, path);
    }

    aria::handle<aria::material_asset> import_material(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);

        return ctx.aria_system->import_material(guid, path);
    }

    // 🚀 Yeni Eklenen Shader Import Kapısı
    aria::handle<aria::shader_asset> import_shader(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);

        return ctx.aria_system->import_shader(guid, path);
    }

    // --- Primitifler (Sanal yollar adres defterine yazılmaz, çünkü diske bağımlı değiller) ---
    aria::handle<aria::model_asset> create_cube_mesh(EngineContext& ctx)
    {
        if (!ctx.aria_system) return {};
        return ctx.aria_system->import_virtual_model("engine://builtin/cube");
    }

    aria::handle<aria::model_asset> create_sphere_mesh(EngineContext& ctx)
    {
        if (!ctx.aria_system) return {};
        return ctx.aria_system->import_virtual_model("engine://builtin/sphere");
    }
}