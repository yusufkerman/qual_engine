#pragma once

/**
 * @file gameplay_entity.h
 * @brief Varlıklar (Entity) üzerinde nesne yönelimli, güvenli ve kolay işlem yapılmasını sağlayan tutucu (handle).
 */

#include <string>
#include <cassert>
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include "transform_wrapper.h"
#include "rigidbody_wrapper.h"

namespace gameplay
{
    /**
     * @class gameplay_entity
     * @brief Ham ECS varlık kimliğini (entity) sararak bileşen erişimi, isim yönetimi ve yaşam döngüsü işlevleri sunar.
     */
    class gameplay_entity
    {
    public:
        /**
         * @brief Geçersiz (boş) bir varlık tutucu oluşturur.
         */
        gameplay_entity() = default;

        /**
         * @brief Belirtilen varlık kimliği ve kayıt defteri ile tutucuyu oluşturur.
         * @param id Varlık kimliği.
         * @param reg Sahne kayıt defteri.
         */
        gameplay_entity(entity id, registry* reg);

        /**
         * @brief Varlık kimliğini döner.
         */
        [[nodiscard]] entity id() const noexcept { return m_id; }

        /**
         * @brief Varlığın sahnede geçerli olup olmadığını doğrular.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Doğrudan boolean koşullarında geçerlilik kontrolü sağlar (if (ent) ...).
         */
        explicit operator bool() const noexcept { return is_valid(); }

        bool operator==(const gameplay_entity& other) const noexcept
        {
            return m_id == other.m_id && m_reg == other.m_reg;
        }

        bool operator!=(const gameplay_entity& other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * @brief Varlığın görünen adını (name_component) döner.
         */
        [[nodiscard]] std::string get_name() const;

        /**
         * @brief Varlığın görünen adını günceller.
         * @param name Yeni varlık adı.
         */
        void set_name(const std::string& name);

        /**
         * @brief Varlığın uzamsal dönüşüm (Transform) sarmalayıcısına erişir.
         */
        [[nodiscard]] transform_wrapper transform();

        /**
         * @brief Varlığın fizik gövdesi (Rigidbody) sarmalayıcısına erişir.
         */
        [[nodiscard]] rigidbody_wrapper rigidbody();

        /**
         * @brief Varlığın belirtilen bileşene sahip olup olmadığını kontrol eder.
         * @tparam T Sorgulanacak bileşen tipi.
         */
        template<typename T>
        [[nodiscard]] bool has_component() const
        {
            return is_valid() && m_reg->has_component<T>(m_id);
        }

        /**
         * @brief Varlığın belirtilen bileşenine referans döner. Bileşen yoksa assert üretir.
         * @tparam T İstenen bileşen tipi.
         */
        template<typename T>
        [[nodiscard]] T& get_component()
        {
            assert(is_valid() && "gameplay_entity::get_component: Gecersiz varlik!");
            T* ptr = m_reg->get_component<T>(m_id);
            assert(ptr && "gameplay_entity::get_component: Bilesen bulunamadi!");
            return *ptr;
        }

        /**
         * @brief Varlığın belirtilen bileşenine güvenli işaretçi döner. Yoksa nullptr döner.
         * @tparam T İstenen bileşen tipi.
         */
        template<typename T>
        [[nodiscard]] T* try_get_component()
        {
            if (!is_valid())
            {
                return nullptr;
            }
            return m_reg->get_component<T>(m_id);
        }

        /**
         * @brief Varlığa yeni bir bileşen ekler.
         * @tparam T Eklenecek bileşen tipi.
         * @tparam Args Bileşen kurucu argüman tipleri.
         */
        template<typename T, typename... Args>
        T& add_component(Args&&... args)
        {
            assert(is_valid() && "gameplay_entity::add_component: Gecersiz varlik!");
            return m_reg->add_component<T>(m_id, std::forward<Args>(args)...);
        }

        /**
         * @brief Varlıktan belirtilen bileşeni kaldırır.
         * @tparam T Kaldırılacak bileşen tipi.
         */
        template<typename T>
        void remove_component()
        {
            if (is_valid())
            {
                m_reg->remove_component<T>(m_id);
            }
        }

        /**
         * @brief Bağlı olduğu kayıt defteri işaretçisini döner.
         */
        [[nodiscard]] registry* get_registry() const noexcept { return m_reg; }

        /**
         * @brief Varlığı sahneden silinmek üzere işaretler / yok eder.
         */
        void destroy();

    private:
        entity m_id = 0;
        registry* m_reg = nullptr;
    };
}
