#pragma once

/**
 * @file render_backend.h
 * @brief Grafik API'sinden (OpenGL, Vulkan vb.) bağımsız, soyut render arka uç (backend) arayüzünü tanımlar.
 */

#include "asset_raw_structures.h"
#include <vector>
#include <glm/glm.hpp>

namespace aria {

    // ========================================================================
    // API BAĞIMSIZ GPU KAYNAK KİMLİKLERİ (HANDLES)
    // ========================================================================

    /// GPU üzerindeki doku nesnesini temsil eden kimlik.
    using gpu_texture_id = unsigned int;

    /// GPU üzerinde derlenmiş gölgelendirici (shader) programını temsil eden kimlik.
    using gpu_shader_id = unsigned int;

    /**
     * @struct gpu_mesh_id
     * @brief GPU'ya yüklenmiş mesh tamponlarının donanım kimliklerini tutar.
     */
    struct gpu_mesh_id {
        unsigned int vao_or_buffer_id{ 0 }; ///< Vertex Array Object (veya API'ye eşdeğer tampon kimliği).
        unsigned int vbo{ 0 };              ///< Vertex Buffer Object kimliği.
        unsigned int ebo{ 0 };              ///< Element (Index) Buffer Object kimliği.
    };

    /**
     * @struct mesh_bounds
     * @brief Mesh'in görünürlük testlerinde (frustum culling) kullanılacak yerel sınır verilerini tutar.
     */
    struct mesh_bounds {
        glm::vec3 local_center{ 0.0f };     ///< Yerel merkez noktası.
        float local_radius{ 0.0f };         ///< Sınırlayıcı küre yarıçapı.
    };

    /**
     * @struct submesh_bridge
     * @brief Donanım kimlikleri (GPU) ile matematiksel sınır (CPU) verilerini bir araya getiren taşıyıcı yapı.
     */
    struct submesh_bridge {
        gpu_mesh_id gpu_data;               ///< Çizim için gereken donanım kimlikleri.
        mesh_bounds bounds;                 ///< Görünürlük hesaplamaları için gereken geometrik sınırlar.
    };

    // ========================================================================
    // SOYUT ARKA UÇ ARAYÜZÜ (ABSTRACT BACKEND INTERFACE)
    // ========================================================================

    /**
     * @class render_backend
     * @brief Farklı grafik API'lerinin uygulayacağı temel sözleşmeyi (contract) tanımlayan soyut sınıf.
     */
    class render_backend {
    public:
        virtual ~render_backend() = default;

        /**
         * @brief Grafik API durum makinesini başlatır ve gerekli global ayarları yapar.
         */
        virtual void init() = 0;

        /**
         * @brief Tüm grafikle ilişkili kaynakları serbest bırakarak sistemi kapatır.
         */
        virtual void shutdown() = 0;

        // --- YÜKLEME FONKSİYONLARI ---

        [[nodiscard]] virtual gpu_texture_id create_texture(const raw_texture_data& raw_data) = 0;
        [[nodiscard]] virtual std::vector<submesh_bridge> create_model(const raw_model_data& raw_data) = 0;
        [[nodiscard]] virtual std::vector<submesh_bridge> create_skinned_model(const raw_skinned_model_data& raw_data) = 0;
        [[nodiscard]] virtual gpu_shader_id create_shader(const raw_shader_data& raw_data) = 0;

        // --- YEDEK (FALLBACK) KAYNAK ÜRETİMİ ---

        [[nodiscard]] virtual gpu_texture_id create_fallback_texture() = 0;
        [[nodiscard]] virtual gpu_shader_id create_fallback_shader() = 0;

        // --- SİLME (DESTROY) FONKSİYONLARI ---

        virtual void destroy_texture(gpu_texture_id id) = 0;
        virtual void destroy_model(const std::vector<gpu_mesh_id>& mesh_ids) = 0;
        virtual void destroy_shader(gpu_shader_id id) = 0;

        /**
         * @brief Mevcut GPU durum makinesini (state machine) varsayılan değerlere sıfırlar.
         */
        virtual void reset_state() = 0;
    };

} // namespace aria