#pragma once

/**
 * @file asset_pipeline.h
 * @brief Oyun motoru varlık (asset) boru hattı fonksiyonlarının tanımlarını içerir.
 */

#include <string>
#include <vector>
#include <cstdint>
#include "../Engine/Assets/aria.h"

struct EngineContext;

namespace asset_pipeline
{
    /**
     * @brief Belirtilen varlık için .meta dosyasından UUID okur, yoksa yeni bir tane üretir.
     *
     * @param asset_path Varlığın dosya sistemindeki yolu.
     * @return aria::asset_guid Varlığa ait geçerli UUID.
     */
    aria::asset_guid get_or_create_meta_guid(const std::string& asset_path);

    // --- Diske Bağımlı Varlık (Asset) İçe Aktarma İşlevleri ---

    /**
     * @brief Dışarıdan gelen gerçek model dosyalarını Aria'ya kaydeder (UUID ile).
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Modelin dosya yolu.
     * @return aria::handle<aria::model_asset> İçe aktarılan modele ait handle.
     */
    aria::handle<aria::model_asset>    import_model(aria::system* aria_system, const std::string& path);

    /**
     * @brief İskeletli 3D model dosyasını Aria'ya kaydeder.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Modelin dosya yolu.
     * @return aria::handle<aria::skinned_model_asset> İçe aktarılan derili modele ait handle.
     */
    aria::handle<aria::skinned_model_asset>  import_skinned_model(aria::system* aria_system, const std::string& path);

    /**
     * @brief İskelet dosyasını Aria'ya kaydeder.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path İskelet dosya yolu.
     * @return aria::handle<aria::skeleton_asset> İçe aktarılan iskelete ait handle.
     */
    aria::handle<aria::skeleton_asset>       import_skeleton(aria::system* aria_system, const std::string& path);

    /**
     * @brief Animasyon klibi dosyasını Aria'ya kaydeder.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Animasyon dosya yolu.
     * @return aria::handle<aria::animation_clip_asset> İçe aktarılan animasyona ait handle.
     */
    aria::handle<aria::animation_clip_asset> import_animation(aria::system* aria_system, const std::string& path);

    /**
     * @brief Dışarıdan gelen gerçek doku (texture) dosyalarını Aria'ya kaydeder.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param path Dokunun dosya yolu.
     * @return aria::handle<aria::texture_asset> İçe aktarılan dokuya ait handle.
     */
    aria::handle<aria::texture_asset>  import_texture(EngineContext& ctx, const std::string& path);

    /**
     * @brief Dışarıdan gelen gerçek materyal dosyalarını Aria'ya kaydeder.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @param path Materyalin dosya yolu.
     * @return aria::handle<aria::material_asset> İçe aktarılan materyale ait handle.
     */
    aria::handle<aria::material_asset> import_material(aria::system* aria_system, const std::string& path);
    aria::handle<aria::material_asset> reload_material(aria::system* aria_system, const std::string& path);

    /**
     * @brief Dışarıdan gelen gerçek shader dosyalarını Aria'ya kaydeder.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param path Shader dosya yolu.
     * @return aria::handle<aria::shader_asset> İçe aktarılan shader'a ait handle.
     */
    aria::handle<aria::shader_asset>   import_shader(EngineContext& ctx, const std::string& path);

    // --- Dahili Primitif Üretim İşlevleri ---

    /**
     * @brief Sanal yol (virtual path) kullanarak Aria üzerinden dahili küp mesh'i üretir.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @return aria::handle<aria::model_asset> Küp modeline ait handle.
     */
    aria::handle<aria::model_asset>   create_cube_mesh(aria::system* aria_system);

    /**
     * @brief Sanal yol (virtual path) kullanarak Aria üzerinden dahili küre mesh'i üretir.
     *
     * @param aria_system Kullanılacak aria sistem nesnesi.
     * @return aria::handle<aria::model_asset> Küre modeline ait handle.
     */
    aria::handle<aria::model_asset>   create_sphere_mesh(aria::system* aria_system);
}