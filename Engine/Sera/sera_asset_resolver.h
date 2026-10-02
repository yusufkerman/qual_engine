#pragma once

/**
 * @file sera_asset_resolver.h
 * @brief SERA ile ARIA (Asset Registry) arasındaki tek yönlü soyut köprü katmanı.
 */

#include <string>
#include "../Engine/engine_context.h"

namespace sera {

    /**
     * @class sera_asset_resolver
     * @brief Sahne verilerindeki varlık tanıtıcılarını (handle) kalıcı kimliklere (GUID) çeviren ve yükleme sırasında tersini yapan servis.
     */
    class sera_asset_resolver
    {
    public:
        /**
         * @brief Bağımlılığı en aza indirmek için motorun context yapısını referans olarak alır.
         * @param ctx ARIA sistemine erişimi sağlayacak motor bağlamı.
         */
        explicit sera_asset_resolver(EngineContext& ctx) : m_ctx(ctx) {}
        ~sera_asset_resolver() = default;

        // Kopyalama işlemleri referans tutulduğu için engellenmelidir (İsteğe bağlı güvenlik)
        sera_asset_resolver(const sera_asset_resolver&) = delete;
        sera_asset_resolver& operator=(const sera_asset_resolver&) = delete;

        /**
         * @brief Canlı bir ARIA handle'ını disk için kalıcı string GUID'ye çevirir.
         * @note ARIA sistemine generic köprü atar.
         *
         * @tparam T Varlık tipi (örn: aria::model_asset, aria::texture_asset).
         * @param handle Çevrilecek çalışma zamanı varlık tanıtıcısı.
         * @return Varlığın kalıcı GUID değeri (Handle geçersizse boş döner).
         */
        template<typename T>
        [[nodiscard]] aria::asset_guid to_guid(const aria::handle<T>& handle) const
        {
            if (!handle.is_valid() || !m_ctx.aria_system)
            {
                return {};
            }

            return m_ctx.aria_system->get_guid_by_handle<T>(handle);
        }

        /**
         * @brief Diskten okunan kalıcı GUID'yi ARIA'ya otomatik yükletip taze bir runtime handle alır.
         *
         * @tparam T Varlık tipi (örn: aria::model_asset, aria::texture_asset).
         * @param guid Diskteki kayıtlı varlık kimliği.
         * @return Yüklenmiş veya önbellekteki varlığın çalışma zamanı tanıtıcısı.
         */
        template<typename T>
        [[nodiscard]] aria::handle<T> to_handle(const aria::asset_guid& guid)
        {
            if (guid.is_nil() || !m_ctx.aria_system)
            {
                return {};
            }

            return m_ctx.aria_system->get_or_load_asset_by_guid<T>(guid);
        }

    private:
        EngineContext& m_ctx; ///< Motor sistemlerine ve ARIA kayıt defterine erişim bağlamı.
    };

} // namespace sera