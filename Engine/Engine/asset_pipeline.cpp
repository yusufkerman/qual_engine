/**
 * @file asset_pipeline.h
 * @brief Oyun motoru için varlık (asset) içe aktarma ve yönetim işlevlerini içerir.
 */

#include "asset_pipeline.h"
#include "engine_context.h"
#include "../Resources/engine_resources.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../Externals/uuid/uuid.h" 
#include "../Externals/nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace asset_pipeline
{
    /**
     * @brief Belirtilen varlık için .meta dosyasından UUID okur, yoksa yeni bir tane üretir.
     *
     * @param asset_path Varlığın dosya sistemindeki yolu.
     * @return aria::asset_guid Varlığa ait geçerli UUID.
     */
    aria::asset_guid get_or_create_meta_guid(const std::string& asset_path)
    {
        // 1. Motor dahili kaynağı ise sabit deterministik GUID'i doğrudan dön, diske meta yazma
        if (auto builtin = engine_resources::get_builtin_guid_for_path(asset_path)) {
            return *builtin;
        }

        std::string meta_path = asset_path + ".meta";

        if (fs::exists(meta_path)) {
            try {
                std::ifstream file(meta_path);
                json meta_json;
                file >> meta_json;
                file.close();

                std::string guid_str = meta_json.value("guid", "");
                auto parsed_guid = uuids::uuid::from_string(guid_str);
                if (parsed_guid) {
                    return *parsed_guid;
                }
            }
            catch (...) {
                std::cout << "[AssetPipeline] Uyari: Meta dosyasi okunurken hata olustu, yeniden uretilecek: " << meta_path << "\n";
            }
        }

        std::random_device rd;
        auto seed_data = std::array<int, std::numeric_limits<int>::digits>{};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 engine(seq);
        uuids::uuid_random_generator gen(engine);
        aria::asset_guid yeni_guid = gen();

        try {
            json meta_json;
            meta_json["guid"] = uuids::to_string(yeni_guid);

            std::ofstream file(meta_path);
            file << meta_json.dump(4);
            file.close();
            std::cout << "[AssetPipeline] Yeni .meta dosyasi olusturuldu: " << meta_path << "\n";
        }
        catch (...) {
            std::cout << "[AssetPipeline] Hata: .meta dosyasi diske yazilamadi!\n";
        }

        return yeni_guid;
    }

    /**
     * @brief 3D model dosyasını sisteme içe aktarır.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Modelin dosya yolu.
     * @return aria::handle<aria::model_asset> İçe aktarılan modele ait handle.
     */
    aria::handle<aria::model_asset> import_model(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->import_model(guid, path);
    }

    /**
     * @brief İskeletli 3D model dosyasını sisteme içe aktarır.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Modelin dosya yolu.
     * @return aria::handle<aria::skinned_model_asset> İçe aktarılan modele ait handle.
     */
    aria::handle<aria::skinned_model_asset> import_skinned_model(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->import_skinned_model(guid, path);
    }

    /**
     * @brief İskelet dosyasını sisteme içe aktarır.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path İskeletin dosya yolu.
     * @return aria::handle<aria::skeleton_asset> İçe aktarılan iskelete ait handle.
     */
    aria::handle<aria::skeleton_asset> import_skeleton(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->import_skeleton(guid, path);
    }

    /**
     * @brief Animasyon klibi dosyasını sisteme içe aktarır.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Animasyon dosya yolu.
     * @return aria::handle<aria::animation_clip_asset> İçe aktarılan animasyona ait handle.
     */
    aria::handle<aria::animation_clip_asset> import_animation(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->import_animation(guid, path);
    }

    /**
     * @brief Doku (texture) dosyasını sisteme içe aktarır.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param path Dokunun dosya yolu.
     * @return aria::handle<aria::texture_asset> İçe aktarılan dokuya ait handle.
     */
    aria::handle<aria::texture_asset> import_texture(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return ctx.aria_system->import_texture(guid, path);
    }

    /**
     * @brief Materyal dosyasını sisteme içe aktarır.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Materyalin dosya yolu.
     * @return aria::handle<aria::material_asset> İçe aktarılan materyale ait handle.
     */
    aria::handle<aria::material_asset> import_material(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->import_material(guid, path);
    }

    aria::handle<aria::material_asset> reload_material(aria::system* aria_system, const std::string& path)
    {
        if (!aria_system) return {};

        aria::asset_guid guid = get_or_create_meta_guid(path);
        return aria_system->reload_material(guid, path);
    }

    /**
     * @brief Shader dosyasını sisteme içe aktarır.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param path Shader dosya yolu.
     * @return aria::handle<aria::shader_asset> İçe aktarılan shader'a ait handle.
     */
    aria::handle<aria::shader_asset> import_shader(EngineContext& ctx, const std::string& path)
    {
        if (!ctx.aria_system) return {};

        std::string resolved = engine_resources::resolve_path(path);
        aria::asset_guid guid = get_or_create_meta_guid(path);
        return ctx.aria_system->import_shader(guid, resolved);
    }

    /**
     * @brief Motorun dahili küp mesh'ini (primitif) oluşturur.
     * @note Sanal yollar diske bağımlı olmadığı için adres defterine yazılmaz.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @return aria::handle<aria::model_asset> Küp modeline ait handle.
     */
    aria::handle<aria::model_asset> create_cube_mesh(aria::system* aria_system)
    {
        if (!aria_system) return {};
        return aria_system->import_virtual_model("engine://builtin/cube");
    }

    /**
     * @brief Motorun dahili küre mesh'ini (primitif) oluşturur.
     * @note Sanal yollar diske bağımlı olmadığı için adres defterine yazılmaz.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @return aria::handle<aria::model_asset> Küre modeline ait handle.
     */
    aria::handle<aria::model_asset> create_sphere_mesh(aria::system* aria_system)
    {
        if (!aria_system) return {};
        return aria_system->import_virtual_model("engine://builtin/sphere");
    }
}