#pragma once

/**
 * @file asset_raw_structures.h
 * @brief Diskten belleğe (CPU) yüklenen varlıkların ham (raw) veri yapılarını barındırır.
 * @note Bu yapılar, veriler GPU'ya aktarılana kadar geçici taşıyıcı görevi görürler.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../Externals/uuid/uuid.h"

 // ---------------------------------------------------------
 // ÖN BİLDİRİMLER (FORWARD DECLARATIONS) VE TİP TANIMLAMALARI
 // ---------------------------------------------------------

namespace uuids {
    class uuid;
}

namespace aria {

    /**
     * @typedef asset_guid
     * @brief Motor genelinde varlıkları benzersiz olarak tanımlayan GUID türü (stduuid sarmalayıcısı).
     */
    using asset_guid = uuids::uuid;

    /**
     * @enum render_mode
     * @brief Materyalin çizim aşamasındaki saydamlık davranışını belirler.
     */
    enum class render_mode : int {
        opaque = 0, ///< Düz, saydamlık içermeyen nesneler (Örn: Duvar, taş).
        cutout = 1, ///< Keskin alfa testi (Alpha test) gerektiren nesneler (Örn: Yaprak, çit).
        transparent = 2  ///< Yarı saydam (Alpha blend) nesneler (Örn: Cam, su).
    };

    // ---------------------------------------------------------
    // HAM VARLIK YAPILARI (RAW DATA STRUCTURES)
    // ---------------------------------------------------------

    /**
     * @struct raw_texture_data
     * @brief Diskten okunan piksel verilerini CPU belleğinde tutar.
     */
    struct raw_texture_data {
        std::string debug_path;         ///< Varlığın yüklendiği dosya yolu (Hata ayıklama amaçlı).
        int width{ 0 };                 ///< Dokunun piksel cinsinden genişliği.
        int height{ 0 };                ///< Dokunun piksel cinsinden yüksekliği.
        int channels{ 0 };              ///< Renk kanalı sayısı (3: RGB, 4: RGBA vb.).
        std::vector<uint8_t> pixels;    ///< Ham piksel byte dizisi.

        /**
         * @brief GPU aktarımı tamamlandıktan sonra vektör kapasitesini sıfırlayarak RAM'i temizler.
         */
        void clear_raw_data() {
            pixels.clear();
            pixels.shrink_to_fit();
        }
    };

    /**
     * @struct raw_vertex
     * @brief Tekil bir köşeye (vertex) ait temel 3D model verilerini tutar.
     */
    struct raw_vertex {
        glm::vec3 position{ 0.0f };     ///< 3D uzaydaki konum.
        glm::vec3 normal{ 0.0f };       ///< Yüzey normali vektörü (Işıklandırma için).
        glm::vec2 tex_coord{ 0.0f };    ///< UV kaplama koordinatları.
        glm::vec3 tangent{ 0.0f };      ///< Tanjant vektörü (Normal mapping için).
    };

    /**
     * @struct raw_mesh_data
     * @brief Alt mesh (sub-mesh) verilerini ve yerel sınır (bounding) hesaplamalarını barındırır.
     */
    struct raw_mesh_data {
        std::vector<raw_vertex> vertices; ///< Köşe noktaları dizisi.
        std::vector<uint32_t> indices;    ///< Köşe indeksleri dizisi (Çizim sırası).

        glm::vec3 local_center{ 0.0f }; ///< Mesh'in yerel merkez noktası.
        float local_radius{ 0.0f };     ///< Mesh'in yerel sınırlayıcı küre yarıçapı (Frustum culling için).

        /**
         * @brief GPU aktarımı tamamlandıktan sonra vektörleri ve kapasitelerini sıfırlar.
         */
        void clear_raw_data() {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
        }
    };

    /**
     * @struct raw_model_data
     * @brief Bir veya daha fazla alt mesh'ten oluşan ana 3D model yapısı.
     */
    struct raw_model_data {
        std::string debug_path;                     ///< Modelin yüklendiği dosya yolu.
        std::vector<raw_mesh_data> sub_meshes;      ///< Modele ait alt mesh (sub-mesh) koleksiyonu.

        /**
         * @brief Modele ait tüm alt meshlerin ham verilerini bellekten temizler.
         */
        void clear_raw_data() {
            for (auto& mesh : sub_meshes) {
                mesh.clear_raw_data();
            }
            sub_meshes.clear();
            sub_meshes.shrink_to_fit();
        }
    };

    // =========================================================
    // İSKELET VE ANİMASYON HAM VERİ YAPILARI (SKELETAL & ANIMATION)
    // =========================================================

    inline constexpr int MAX_BONES_PER_VERTEX = 4;
    inline constexpr int MAX_BONES_PER_SKELETON = 128;

    /**
     * @struct raw_skinned_vertex
     * @brief İskeletli modeller için kemik indeks ve ağırlıklarını barındıran köşe yapısı.
     */
    struct raw_skinned_vertex {
        glm::vec3 position{ 0.0f };                           ///< 3D uzaydaki konum.
        glm::vec3 normal{ 0.0f };                             ///< Yüzey normali vektörü (Işıklandırma için).
        glm::vec2 tex_coord{ 0.0f };                          ///< UV kaplama koordinatları.
        glm::vec3 tangent{ 0.0f };                            ///< Tanjant vektörü (Normal mapping için).
        glm::ivec4 bone_ids{ -1, -1, -1, -1 };                ///< Bu köşeyi etkileyen kemiklerin indeksleri (Maksimum 4 kemik).
        glm::vec4 bone_weights{ 0.0f, 0.0f, 0.0f, 0.0f };     ///< Her kemiğin etki ağırlığı (Toplamları 1.0).
    };

    /**
     * @struct raw_skinned_mesh_data
     * @brief İskeletli alt mesh verilerini ve yerel sınır hesaplamalarını barındırır.
     */
    struct raw_skinned_mesh_data {
        std::vector<raw_skinned_vertex> vertices; ///< İskeletli köşe noktaları dizisi.
        std::vector<uint32_t> indices;            ///< Köşe indeksleri dizisi (Çizim sırası).

        glm::vec3 local_center{ 0.0f };           ///< Mesh'in yerel merkez noktası.
        float local_radius{ 0.0f };               ///< Mesh'in yerel sınırlayıcı küre yarıçapı.

        /**
         * @brief GPU aktarımı tamamlandıktan sonra vektörleri ve kapasitelerini sıfırlar.
         */
        void clear_raw_data() {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
        }
    };

    /**
     * @struct bone_info
     * @brief Tek bir kemiğin indeks, isim ve ters bağlanma (Inverse Bind Pose) matrisini tutar.
     */
    struct bone_info {
        int id{ -1 };                         ///< Matris paletindeki benzersiz kemik indeksi (0..MAX_BONES_PER_SKELETON-1).
        std::string name;                     ///< Kemiğin adı (Örn: "mixamorig:RightArm").
        glm::mat4 offset_matrix{ 1.0f };      ///< Mesh uzayından kemik yerel uzayına geçiren Inverse Bind Pose matrisi.
    };

    /**
     * @struct skeleton_node
     * @brief İskelet hiyerarşik ağacındaki tekil bir düğüm (Kemik veya yardımcı sahne düğümü).
     */
    struct skeleton_node {
        std::string name;                     ///< Düğümün adı.
        int bone_index{ -1 };                 ///< Eğer kemik ise bone_info indeksi, değilse -1.
        glm::mat4 local_transform{ 1.0f };    ///< Ebeveyn düğüme göre yerel dönüşüm matrisi.
        std::vector<skeleton_node> children;  ///< Alt çocuk düğümler.
    };

    /**
     * @struct raw_skeleton_data
     * @brief İskelet hiyerarşisini, kemik listesini ve isim haritasını barındırır.
     */
    struct raw_skeleton_data {
        std::string debug_path;                               ///< İskeletin yüklendiği dosya yolu.
        skeleton_node root_node;                              ///< İskelet hiyerarşisinin kök düğümü.
        std::vector<bone_info> bones;                         ///< Düz kemik listesi.
        std::unordered_map<std::string, int> bone_mapping;    ///< İsimden kemik indeksine O(1) haritalama.
        glm::mat4 global_inverse_transform{ 1.0f };           ///< Kök düğümün global ters dönüşüm matrisi.

        [[nodiscard]] int find_bone(const std::string& bone_name) const {
            auto it = bone_mapping.find(bone_name);
            return (it != bone_mapping.end()) ? it->second : -1;
        }

        void clear_raw_data() {
            bones.clear();
            bones.shrink_to_fit();
            bone_mapping.clear();
            clear_node_children(root_node);
        }

    private:
        static void clear_node_children(skeleton_node& node) {
            for (auto& child : node.children) {
                clear_node_children(child);
            }
            node.children.clear();
            node.children.shrink_to_fit();
        }
    };

    /**
     * @struct vector_keyframe
     * @brief Konum veya ölçeklendirme için zaman damgalı 3D vektör anahtar karesi.
     */
    struct vector_keyframe {
        float time_stamp{ 0.0f };   ///< Tick veya saniye cinsinden zaman.
        glm::vec3 value{ 0.0f };    ///< Değer (Konum veya ölçek).
    };

    /**
     * @struct quat_keyframe
     * @brief Rotasyon için zaman damgalı kuaterniyon anahtar karesi.
     */
    struct quat_keyframe {
        float time_stamp{ 0.0f };
        glm::quat value{ 1.0f, 0.0f, 0.0f, 0.0f };
    };

    /**
     * @struct bone_animation_channel
     * @brief Tekil bir kemiğe ait tüm hareket izleri (Position, Rotation, Scale).
     */
    struct bone_animation_channel {
        std::string bone_name;
        std::vector<vector_keyframe> positions;
        std::vector<quat_keyframe>   rotations;
        std::vector<vector_keyframe> scalings;
    };

    /**
     * @struct raw_animation_data
     * @brief CPU belleğinde tutulan tekil bir animasyon klibi verisi.
     */
    struct raw_animation_data {
        std::string debug_path;
        std::string name;
        float duration{ 0.0f };               ///< Klibin toplam süresi (Tick cinsinden).
        float ticks_per_second{ 25.0f };       ///< Saniyedeki tick frekansı.
        std::vector<bone_animation_channel> channels;
        std::unordered_map<std::string, size_t> channel_mapping;

        [[nodiscard]] const bone_animation_channel* find_channel(const std::string& bone_name) const {
            auto it = channel_mapping.find(bone_name);
            return (it != channel_mapping.end()) ? &channels[it->second] : nullptr;
        }

        void clear_raw_data() {
            channels.clear();
            channels.shrink_to_fit();
            channel_mapping.clear();
        }
    };

    /**
     * @struct raw_skinned_model_data
     * @brief İskeletli alt mesh'leri, iskelet hiyerarşisini ve gömülü animasyonları barındıran model yapısı.
     */
    struct raw_skinned_model_data {
        std::string debug_path;
        std::vector<raw_skinned_mesh_data> sub_meshes;
        raw_skeleton_data skeleton;
        std::vector<raw_animation_data> animations;

        [[nodiscard]] bool has_skeleton() const {
            return !skeleton.bones.empty();
        }

        void clear_raw_data() {
            for (auto& mesh : sub_meshes) {
                mesh.clear_raw_data();
            }
            sub_meshes.clear();
            sub_meshes.shrink_to_fit();
            skeleton.clear_raw_data();
            for (auto& anim : animations) {
                anim.clear_raw_data();
            }
            animations.clear();
            animations.shrink_to_fit();
        }
    };

    /**
     * @struct raw_material_data
     * @brief Materyal dosyasından (JSON vb.) okunan görsel özellikleri ve bağlı dokuların GUID'lerini tutar.
     */
    struct raw_material_data {
        std::string debug_path;                     ///< Materyal dosyasının yolu.
        std::string name;                           ///< Materyal adı.

        render_mode mode{ render_mode::opaque };    ///< Çizim saydamlık modu.
        float alpha_threshold{ 0.1f };              ///< Cutout modu için alfa kesme eşiği.

        glm::vec3 base_color{ 1.0f };               ///< Temel renk çarpanı (Albedo).
        float metallic{ 0.0f };                     ///< Metaliklik çarpanı (PBR).
        float roughness{ 0.5f };                    ///< Yüzey pürüzlülüğü (PBR).
        float ao{ 1.0f };                           ///< Ortam örtme çarpanı (Ambient Occlusion).
        glm::vec3 emissive_color{ 0.0f };           ///< Kendiliğinden ışıma rengi.
        float emissive_intensity{ 0.0f };           ///< Işıma şiddeti çarpanı.

        float specular_strength{ 0.5f };            ///< Parlaklık/yansıma şiddeti (Legacy).
        float shininess{ 32.0f };                   ///< Parlaklık pürüzlülük/odak değeri (Legacy).
        float normal_strength{ 1.0f };              ///< Normal haritası etki şiddeti.
        glm::vec2 texture_scale{ 1.0f };            ///< UV ölçeklendirme çarpanı (Tiling).

        aria::asset_guid shader_guid;               ///< Kullanılacak shader programının benzersiz kimliği.
        aria::asset_guid diffuse_texture_guid;      ///< Difüz (Albedo) dokusunun benzersiz kimliği.
        aria::asset_guid normal_texture_guid;       ///< Normal dokusunun benzersiz kimliği.
        aria::asset_guid metallic_texture_guid;     ///< Metalik dokusunun benzersiz kimliği (PBR).
        aria::asset_guid roughness_texture_guid;    ///< Pürüzlülük dokusunun benzersiz kimliği (PBR).
        aria::asset_guid ao_texture_guid;           ///< Ambient Occlusion dokusunun benzersiz kimliği (PBR).
        aria::asset_guid emissive_texture_guid;     ///< Işıma (Emissive) dokusunun benzersiz kimliği (PBR).
        aria::asset_guid specular_texture_guid;     ///< Speküler dokusunun benzersiz kimliği (Legacy).

        /**
         * @brief Materyal verileri oldukça küçüktür (vektör içermez), bu nedenle boşaltma gerektirmez.
         */
        void clear_raw_data() {}
    };

    /**
     * @struct raw_shader_data
     * @brief Shader kod dosyalarından (.vert, .frag) okunan ham metin (string) verilerini barındırır.
     */
    struct raw_shader_data {
        std::string debug_path;         ///< Shader dosyalarının kök yolu.
        std::string vertex_source;      ///< Vertex shader kaynak kodu.
        std::string fragment_source;    ///< Fragment shader kaynak kodu.

        /**
         * @brief GPU derleme aşaması bittikten sonra metin belleklerini serbest bırakır.
         */
        void clear_raw_data() {
            vertex_source.clear();
            vertex_source.shrink_to_fit();
            fragment_source.clear();
            fragment_source.shrink_to_fit();
        }
    };

} // namespace aria