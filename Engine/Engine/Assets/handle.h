#pragma once

/**
 * @file handle.h
 * @brief Varlıklar (assets) için nesilsel (generational) bellek erişim tanıtıcısını tanımlar.
 */

#include <cstdint>
#include <compare>

namespace aria {

    /**
     * @struct handle
     * @brief Varlık kayıt defterindeki (registry) yuvalara güvenli erişim sağlayan zayıf referans (weak reference) yapısı.
     * @note Nesil (generation) kontrolü sayesinde, silinmiş (dangling) belleğe erişim engellenir.
     *
     * @tparam T Bu tanıtıcının işaret ettiği varlık türü.
     */
    template<typename T>
    struct handle {
        uint32_t index{ 0 };      ///< Varlığın kayıt defterindeki yuva (slot) indeksi.
        uint32_t generation{ 0 }; ///< Yuvanın tahsis edilme nesli (Silinip üzerine yazıldıkça artar).

        /**
         * @brief Geçersiz (null) bir tanıtıcı oluşturur.
         */
        constexpr handle() : index(0), generation(0) {}

        /**
         * @brief Belirli bir indeks ve nesil değerine sahip bir tanıtıcı oluşturur.
         *
         * @param idx Yuva indeksi.
         * @param gen Nesil değeri.
         */
        constexpr handle(uint32_t idx, uint32_t gen) : index(idx), generation(gen) {}

        /**
         * @brief Tanıtıcının mantıksal olarak geçerli olup olmadığını kontrol eder.
         *
         * @return bool Eğer nesil değeri 0'dan büyükse true, aksi halde false döner.
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return generation != 0;
        }

        /**
         * @brief Tanıtıcıyı manuel olarak geçersiz (null) hale getirir.
         */
        constexpr void invalidate() noexcept {
            index = 0;
            generation = 0;
        }

        /**
         * @brief C++20 Üç Yönlü Karşılaştırma (Spaceship Operator).
         * @note Derleyici; ==, !=, <, >, <=, >= operatörlerini bu tanım üzerinden otomatik olarak üretir.
         */
        auto operator<=>(const handle&) const = default;
    };

} // namespace aria