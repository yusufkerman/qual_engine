#pragma once

/**
 * @file gameplay_context.h
 * @brief Gameplay API katmanının motor çekirdeğine güvenli ve kontrollü erişim köprüsü.
 */

#include "../ECS/registry.h"

struct EngineContext;
struct InputState;
class physics_system;
class scene;

namespace gameplay
{
    class collision_event_queue;

    /**
     * @class context
     * @brief Gameplay modülünün ihtiyaç duyduğu motor servislerine kontrollü erişim sağlayan bağlam yöneticisi.
     */
    class context
    {
    public:
        /**
         * @brief Motor bağlamını sisteme kaydeder.
         * @param ctx Motor bağlam işaretçisi.
         */
        static void set_engine_context(EngineContext* ctx);

        /**
         * @brief Aktif çalışma zamanı sahnesini kaydeder.
         * @param s Sahne işaretçisi.
         */
        static void set_active_scene(scene* s);

        /**
         * @brief Çarpışma olay kuyruğunu kaydeder.
         * @param q Olay kuyruğu işaretçisi.
         */
        static void set_collision_queue(collision_event_queue* q);

        /**
         * @brief Fizik sistemine doğrudan işaretçi atar (Modül ABI izolasyonu için).
         */
        static void set_physics_system(physics_system* p);

        /**
         * @brief Girdi durumu yapısına doğrudan işaretçi atar (Modül ABI izolasyonu için).
         */
        static void set_input_state(InputState* i);

        /**
         * @brief Sistemin ilklendirilip ilklendirilmediğini döner.
         */
        [[nodiscard]] static bool is_initialized();

        /**
         * @brief Motorun editor modunda mi yoksa standalone oyun modunda mi calistigini doner.
         */
        [[nodiscard]] static bool is_editor_build();

        /**
         * @brief Ham motor bağlamını döner.
         */
        [[nodiscard]] static EngineContext* get_engine_context();

        /**
         * @brief Aktif sahnenin ECS kayıt defterini döner.
         */
        [[nodiscard]] static registry* get_registry();

        /**
         * @brief Aktif fizik sistemini döner.
         */
        [[nodiscard]] static physics_system* get_physics_system();

        /**
         * @brief Aktif girdi durum yapısını döner.
         */
        [[nodiscard]] static InputState* get_input_state();

        /**
         * @brief Aktif çarpışma olay kuyruğunu döner.
         */
        [[nodiscard]] static collision_event_queue* get_collision_queue();

        /**
         * @brief Aktif sahne işaretçisini döner.
         */
        [[nodiscard]] static scene* get_active_scene();

    private:
        static inline EngineContext* s_engine_ctx = nullptr;
        static inline scene* s_active_scene = nullptr;
        static inline collision_event_queue* s_collision_queue = nullptr;
        static inline physics_system* s_physics_system = nullptr;
        static inline InputState* s_input_state = nullptr;
    };
}

