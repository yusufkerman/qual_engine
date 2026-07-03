/**
 * @file sera_session.h
 * @brief SERA sisteminin dış dünyaya açılan ana kontrol ve oturum yönetim kapısı.
 */
#pragma once

#include "sera_types.h"
#include "sera_entity_registry.h"
#include "sera_asset_resolver.h"
#include "../Scene/scene.h"
#include <string>

struct EngineContext;

namespace sera {

    class sera_session {
    public:
        // Oturum başlarken ARIA bağımlılığı için context referansını alıyoruz
        explicit sera_session(EngineContext& ctx);
        ~sera_session() = default;

        /**
         * @brief Mevcut canlı sahneyi diske JSON formatında güvenle serileştirir.
         */
        validation_report save(const std::string& filepath, const scene& current_scene);

        /**
         * @brief Diskten okuduğu JSON verisiyle sahneyi ve UUID nüfus haritalarını sıfırdan ayağa kaldırır.
         */
        validation_report load(const std::string& filepath, scene& out_scene);

        // Nüfus dairesine dışarıdan erişmek gerekirse (Örn: Editör veya script'ler için)
        sera_entity_registry& get_entity_registry() { return m_entity_registry; }
        const sera_entity_registry& get_entity_registry() const { return m_entity_registry; }

    private:
        EngineContext& m_ctx;
        sera_entity_registry m_entity_registry;
        sera_asset_resolver m_asset_resolver;
    };

} // namespace sera