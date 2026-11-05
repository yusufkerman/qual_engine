#pragma once

/**
 * @file mesh_collider_baker.h
 * @brief Hiyerarşik alt ağları (Submeshes) toplayan, basitleştiren ve Jolt Physics Shape üreten yardımcı servis.
 */

#include "../ECS/entity.h"
#include "../ECS/components.h"
#include <glm/glm.hpp>
#include <vector>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

class registry;
namespace aria { class system; }

namespace qual::physics
{
    class mesh_collider_baker
    {
    public:
        /**
         * @brief Bir varlık ve tüm çocuk hiyerarşisindeki alt ağları toplayıp seçilen çözünürlükte pişirir (Bake).
         * @param root Mesh collider bileşenine sahip kök varlık.
         * @param reg ECS kayıt defteri.
         * @param ariaSys Model ve varlık geometrisine erişim sağlayan Aria sistemi.
         * @param col Güncellenecek mesh collider bileşeni.
         * @return Başarılıysa true, hiçbir geometri bulunamadıysa false döner.
         */
        static bool bake(
            entity root,
            registry& reg,
            aria::system* ariaSys,
            mesh_collider_component& col
        );

        /**
         * @brief Pişirilmiş (Cooked) köşe ve üçgen verilerinden Jolt Physics Shape oluşturur.
         * @param col Pişirilmiş köşe ve üçgenleri barındıran mesh collider bileşeni.
         * @param worldScale Varlığın dünya ölçeği (Scale).
         * @return Oluşturulan Jolt Shape tanıtıcısı.
         */
        static JPH::ShapeRefC create_shape(
            const mesh_collider_component& col,
            const glm::vec3& worldScale
        );

        /**
         * @brief Spatial Grid Clustering algoritması ile üçgen sayısını kontrollü biçimde azaltır.
         * @param inVertices Orijinal köşe noktaları.
         * @param inIndices Orijinal üçgen indeksleri.
         * @param cellDivisor Izgara hücre bölücü katsayısı (Daha küçük = daha kaba/az üçgen).
         * @param outVertices Sadeleştirilmiş köşe noktaları çıktısı.
         * @param outIndices Sadeleştirilmiş üçgen indeksleri çıktısı.
         */
        static void simplify_mesh(
            const std::vector<glm::vec3>& inVertices,
            const std::vector<uint32_t>& inIndices,
            float cellDivisor,
            std::vector<glm::vec3>& outVertices,
            std::vector<uint32_t>& outIndices
        );
    };
} // namespace qual::physics

