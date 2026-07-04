#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// 🚀 stduuid kütüphanesinin eklenmesi (asset_guid tanımı için şart)
#include "../Externals/uuid/uuid.h" // Projendeki tam klasör yoluna göre uyarla

// Forward Declarations (Ön Bildirimler)
// İleride asset_guid'i doğrudan namespace kırılımlarında kullanırken 
// derleyicinin "Bu nedir?" dememesi için takma isim (alias) tanımlıyoruz.
namespace uuids {
    class uuid;
}

namespace aria {

    // asset_guid takma adını global güvenli hale getiriyoruz
    using asset_guid = uuids::uuid;


    enum class render_mode : int {
        opaque = 0,      // Düz nesneler (Ev, taş)
        cutout = 1,      // Keskin şeffaflık (Ağaç yaprağı, çimen)
        transparent = 2  // Yarı şeffaflık (Cam, su)
    };

    // Doku kurye yapısı (Orijinal güvenli hali)
    struct raw_texture_data {
        std::string debug_path;
        int width{ 0 };
        int height{ 0 };
        int channels{ 0 };
        std::vector<uint8_t> pixels;

        void clear_raw_data() {
            pixels.clear();
            pixels.shrink_to_fit();
        }
    };

    // Vertex yapısı
    struct raw_vertex {
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f };
        glm::vec2 tex_coord{ 0.0f };
        glm::vec3 tangent{ 0.0f };
    };

    // Tekil Mesh yapısı
    struct raw_mesh_data {
        std::vector<raw_vertex> vertices;
        std::vector<uint32_t> indices;

        glm::vec3 local_center{ 0.0f };
        float local_radius{ 0.0f };

        void clear_raw_data() {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
        }
    };

    // Model Yapısı
    struct raw_model_data {
        std::string debug_path;
        std::vector<raw_mesh_data> sub_meshes;

        void clear_raw_data() {
            for (auto& mesh : sub_meshes) {
                mesh.clear_raw_data();
            }
            sub_meshes.clear();
            sub_meshes.shrink_to_fit();
        }
    };

	// Doğrudan okunacak ham malzeme (material) yapısı
    struct raw_material_data {
        std::string debug_path;
        std::string name;

        render_mode mode{ render_mode::opaque };
        float alpha_threshold{ 0.1f };

        glm::vec3 base_color{ 1.0f };
        float specular_strength{ 0.5f };
        float shininess{ 32.0f };
        float normal_strength{ 1.0f };
        glm::vec2 texture_scale{ 1.0f };

        // Okunan GUID'lerin saklanacağı Aria Tipleri
        aria::asset_guid shader_guid;
        aria::asset_guid diffuse_texture_guid;
        aria::asset_guid normal_texture_guid;
        aria::asset_guid specular_texture_guid;

        void clear_raw_data() {}
    };

    // Shader Yapısı
    struct raw_shader_data {
        std::string debug_path;
        std::string vertex_source;
        std::string fragment_source;

        void clear_raw_data() {
            vertex_source.clear();
            vertex_source.shrink_to_fit();
            fragment_source.clear();
            fragment_source.shrink_to_fit();
        }
    };

} // namespace aria