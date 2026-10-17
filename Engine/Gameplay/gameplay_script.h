#pragma once

/**
 * @file gameplay_script.h
 * @brief Modern, olay güdümlü ve ergonomik yeni nesil oyun betiği (gameplay script) taban sınıfı.
 */

#include "../Scene/scriptable_entity.h"
#include "gameplay_context.h"
#include "collision_types.h"
#include "gameplay_entity.h"
#include "gameplay_scene.h"
#include "gameplay_input.h"

namespace gameplay
{
    /**
     * @class script
     * @brief Kullanıcı tanımlı oyun mantıkları için yaşam döngüsü, çarpışma ve girdi fonksiyonlarını barındıran temel sınıf.
     */
    class script : public scriptable_entity
    {
    public:
        virtual ~script() override = default;

        void setContext(EngineContext* context) override
        {
            scriptable_entity::setContext(context);
            context::set_engine_context(context);
        }

        // ==========================================
        // YAŞAM DÖNGÜSÜ (LIFECYCLE) OLAYLARI
        // ==========================================

        /**
         * @brief Betik ilk kez oluşturulduğunda ve varlığa bağlandığında bir kez çalışır.
         */
        virtual void on_create() {}

        /**
         * @brief Betik veya bağlı olduğu varlık yok edilirken çalışır.
         */
        virtual void on_destroy() {}

        /**
         * @brief Her grafik karesinde değişken delta time ile çağrılır (Görsel ve girdi güncellemeleri için).
         * @param delta_time Son kareden bu yana geçen saniye cinsinden süre.
         */
        virtual void on_update(float delta_time) {}

        /**
         * @brief Sabit fizik adımlarında çağrılır (Kuvvet uygulama, fizik hareketi vb. için).
         * @param fixed_delta_time Sabit simülasyon zaman adımı (ör. 1/60 sn).
         */
        virtual void on_fixed_update(float fixed_delta_time) {}

        // ==========================================
        // ÇARPIŞMA (COLLISION) OLAYLARI
        // ==========================================

        /**
         * @brief Başka bir katı fizik gövdesiyle ilk temas anında tetiklenir.
         * @param hit Çarpışma noktası, normali ve çarpan varlık bilgileri.
         */
        virtual void on_collision_enter(const collision_hit& hit) {}

        /**
         * @brief Fiziksel temas devam ettiği sürece her karede tetiklenir.
         * @param hit Güncel temas bilgileri.
         */
        virtual void on_collision_persist(const collision_hit& hit) {}

        /**
         * @brief Fiziksel temas sonlandığında (nesneler ayrıldığında) tetiklenir.
         * @param hit Ayrılan varlığın bilgileri.
         */
        virtual void on_collision_exit(const collision_hit& hit) {}

        // ==========================================
        // TETİKLEYİCİ (TRIGGER) OLAYLARI
        // ==========================================

        /**
         * @brief Bir tetikleyici (sensor) alana başka bir varlık girdiğinde tetiklenir.
         * @param hit Alana giren varlık bilgisi.
         */
        virtual void on_trigger_enter(const trigger_hit& hit) {}

        /**
         * @brief Bir varlık tetikleyici alandan çıktığında tetiklenir.
         * @param hit Alandan ayrılan varlık bilgisi.
         */
        virtual void on_trigger_exit(const trigger_hit& hit) {}

        // ==========================================
        // ERGONOMİK ERİŞİMCİLER (ACCESSORS)
        // ==========================================

        /**
         * @brief Betiğin bağlı olduğu varlık kimliğini döner.
         */
        [[nodiscard]] entity entity_id() const noexcept { return get_entity(); }

        /**
         * @brief Betiğin bağlı olduğu varlığı nesne tutucusu (gameplay_entity) olarak döner.
         */
        [[nodiscard]] gameplay_entity self() { return gameplay_entity(get_entity(), get_registry()); }

        /**
         * @brief Varlığın uzamsal konum/rotasyon sarmalayıcısına doğrudan erişim sağlar.
         */
        [[nodiscard]] transform_wrapper transform() { return transform_wrapper(get_entity(), get_registry()); }

        /**
         * @brief Varlığın fizik gövdesi sarmalayıcısına doğrudan erişim sağlar.
         */
        [[nodiscard]] rigidbody_wrapper rigidbody() { return rigidbody_wrapper(get_entity(), get_registry()); }

        /**
         * @brief Varlıktaki belirtilen bileşene referans döner.
         */
        template<typename T>
        [[nodiscard]] T& get_component()
        {
            return self().template get_component<T>();
        }

        /**
         * @brief Varlıktaki belirtilen bileşene işaretçi döner. Yoksa nullptr döner.
         */
        template<typename T>
        [[nodiscard]] T* try_get_component()
        {
            return self().template try_get_component<T>();
        }

        /**
         * @brief Varlıkta belirtilen bileşenin olup olmadığını sorgular.
         */
        template<typename T>
        [[nodiscard]] bool has_component() const
        {
            auto* reg = get_registry();
            if (!reg || get_entity() == 0) return false;
            return reg->has_component<T>(get_entity());
        }

        /**
         * @brief Sahnede belirtilen ada sahip varlığı arar.
         * @param name Aranacak varlık adı.
         */
        [[nodiscard]] gameplay_entity find_entity_by_name(const std::string& name)
        {
            return scene_service::find_entity(name);
        }
    };
}

