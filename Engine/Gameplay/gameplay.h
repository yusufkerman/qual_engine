#pragma once

/**
 * @file gameplay.h
 * @brief Gameplay API modülünün tüm başlık dosyalarını tek bir noktadan içeri aktaran ana başlık dosyası.
 * 
 * Bu başlık dosyası dahil edilerek;
 * - gameplay::input (Eylem, tuş ve fare kontrolleri)
 * - gameplay::transform_wrapper (Konum, rotasyon, forward, right, up, look_at)
 * - gameplay::rigidbody_wrapper (Hız, itme, kütle, sürtünme)
 * - gameplay::gameplay_entity (Bileşen yönetimi ve nesne tutucu)
 * - gameplay::scene_service (Varlık arama, oluşturma, silme)
 * - gameplay::script (Genişletilmiş yaşam döngüsü ve olay tabanlı betik sınıfı)
 * - gameplay::collision_hit / trigger_hit (Çarpışma veri yapıları)
 * - gameplay::context (Motor bağlantı köprüsü)
 * yapılarına doğrudan erişilebilir.
 */

#include "collision_types.h"
#include "collision_event_queue.h"
#include "jolt_contact_listener.h"
#include "gameplay_context.h"
#include "gameplay_input.h"
#include "transform_wrapper.h"
#include "rigidbody_wrapper.h"
#include "gameplay_entity.h"
#include "gameplay_scene.h"
#include "gameplay_script.h"
#include "gameplay_gizmos.h"
#include "gameplay_time.h"
#include "gameplay_ui.h"

