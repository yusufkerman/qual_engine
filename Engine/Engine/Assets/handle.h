#pragma once
#include <cstdint>
#include <compare>

namespace aria {

    template<typename T>
    struct handle {
        uint32_t index{ 0 };
        uint32_t generation{ 0 };

        // Geçersiz (null) handle üretmek için sarmalayıcı
        constexpr handle() : index(0), generation(0) {}
        constexpr handle(uint32_t idx, uint32_t gen) : index(idx), generation(gen) {}

        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return generation != 0;
        }

        constexpr void invalidate() noexcept {
            index = 0;
            generation = 0;
        }

        // C++20 Üç Yönlü Karşılaştırma (Spaceship Operator) ile tüm operatörler otomatik üretilir
        auto operator<=>(const handle&) const = default;
    };

} // namespace aria