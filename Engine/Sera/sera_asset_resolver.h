/**
 * @file sera_asset_resolver.h
 * @brief SERA ile ARIA (Asset Registry) arasındaki tek yönlü soyut köprü katmanı.
 */
#pragma once

#include <string>
#include "../Engine/engine_context.h"


namespace sera {

    class sera_asset_resolver {
    public:
        // Bağımlılığı en aza indirmek için motorun context yapısını constructor'da alıyoruz
        explicit sera_asset_resolver(EngineContext& ctx) : m_ctx(ctx) {}
        ~sera_asset_resolver() = default;

        /**
         * @brief Canlı bir ARIA handle'ını disk için kalıcı string GUID'ye çevirir.
         * @note ARIA sistemine generic köprü atar.
         */
        template<typename T>
        aria::asset_guid to_guid(const aria::handle<T>& handle) const
        {
            if (!handle.is_valid() || !m_ctx.aria_system)
                return {};

            return m_ctx.aria_system->get_guid_by_handle<T>(handle);
        }

        /**
         * @brief Diskten okunan string GUID'yi ARIA'ya otomatik yükletip taze bir runtime handle alır.
         */
        template<typename T>
        aria::handle<T> to_handle(const aria::asset_guid& guid)
        {
            if (guid.is_nil() || !m_ctx.aria_system)
                return {};

            return m_ctx.aria_system->get_or_load_asset_by_guid<T>(guid);
        }

    private:
        EngineContext& m_ctx;
    };

} // namespace sera