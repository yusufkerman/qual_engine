#pragma once

/**
 * @file asset_loader.h
 * @brief Varlık (asset) dosyalarını diskten okuyup CPU tarafındaki ham (raw) veri yapılarına dönüştüren yükleyici sınıfı tanımlar.
 */

#include "asset_raw_structures.h"
#include <optional>
#include <string>

namespace aria {

    /**
     * @class asset_loader
     * @brief Çeşitli varlık türlerini (doku, model, materyal, shader) diskten okuyan statik yardımcı (utility) sınıf.
     * @note Sınıf hiçbir durum (state) tutmaz, tüm fonksiyonlar statiktir.
     */
    class asset_loader {
    public:
        /**
         * @brief Görüntü dosyasını diskten okur ve CPU tarafında ham doku verisine dönüştürür.
         *
         * @param file_path Doku dosyasının tam veya göreceli yolu.
         * @return std::optional<raw_texture_data> Yükleme başarılıysa ham doku verisi, aksi halde std::nullopt döner.
         */
        [[nodiscard]] static std::optional<raw_texture_data> load_texture_to_cpu(const std::string& file_path);

        /**
         * @brief 3D model dosyasını (OBJ, FBX vb. - Assimp aracılığıyla) diskten okur ve CPU tarafında ham model verisine dönüştürür.
         *
         * @param file_path Model dosyasının tam veya göreceli yolu.
         * @return std::optional<raw_model_data> Yükleme başarılıysa ham model verisi, aksi halde std::nullopt döner.
         */
        [[nodiscard]] static std::optional<raw_model_data> load_model_to_cpu(const std::string& file_path);

        /**
         * @brief İskelet ve animasyon içeren 3D model dosyasını (Mixamo FBX vb.) diskten okur ve CPU tarafında ham derili model verisine dönüştürür.
         *
         * @param file_path Model dosyasının tam veya göreceli yolu.
         * @return std::optional<raw_skinned_model_data> Yükleme başarılıysa derili model, iskelet ve animasyon verisi döner.
         */
        [[nodiscard]] static std::optional<raw_skinned_model_data> load_skinned_model_to_cpu(const std::string& file_path);

        /**
         * @brief Model dosyasından sadece iskelet hiyerarşisi ve kemik verilerini ayrıştırır.
         *
         * @param file_path Model dosyasının yolu.
         * @return std::optional<raw_skeleton_data> Yükleme başarılıysa iskelet verisi döner.
         */
        [[nodiscard]] static std::optional<raw_skeleton_data> load_skeleton_to_cpu(const std::string& file_path);

        /**
         * @brief FBX veya benzeri animasyon dosyasından gömülü veya salt animasyon kliplerini ayrıştırır.
         *
         * @param file_path Animasyon dosyasının yolu.
         * @return std::optional<std::vector<raw_animation_data>> Dosyadaki tüm animasyon klipleri koleksiyonu.
         */
        [[nodiscard]] static std::optional<std::vector<raw_animation_data>> load_animations_to_cpu(const std::string& file_path);

        /**
         * @brief JSON formatındaki materyal dosyasını diskten okur ve CPU tarafında ham materyal verisine dönüştürür.
         *
         * @param file_path Materyal dosyasının tam veya göreceli yolu.
         * @return std::optional<raw_material_data> Yükleme başarılıysa ham materyal verisi, aksi halde std::nullopt döner.
         */
        [[nodiscard]] static std::optional<raw_material_data> load_material_to_cpu(const std::string& file_path);

        /**
         * @brief Shader kaynak dosyalarını (.vert ve .frag) diskten okur ve CPU tarafında ham shader verisine dönüştürür.
         *
         * @param file_path Shader dosyalarının uzantısız temel yolu (örn. "Shaders/default").
         * @return std::optional<raw_shader_data> Yükleme başarılıysa ham shader verisi, aksi halde std::nullopt döner.
         */
        [[nodiscard]] static std::optional<raw_shader_data> load_shader_to_cpu(const std::string& file_path);

    };

} // namespace aria