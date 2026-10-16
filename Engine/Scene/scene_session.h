#pragma once

/**
 * @file scene_session.h
 * @brief Editör ve çalışma zamanı (runtime) sahne durumlarını yöneten oturum sınıfı.
 *
 * Mimari (Unity tarzı "Single Live World + Snapshot"):
 * - Editör ve oyun AYNI canlı sahne (live scene) üzerinde çalışır. Scene View ve Game View bu tek dünyayı
 *   yalnızca farklı kameralarla çizer; kareler arası hiçbir kopyalama / senkronizasyon yapılmaz.
 * - PLAY'e geçerken sahnenin TEK SEFERLİK bir anlık görüntüsü (snapshot) alınır.
 * - STOP'ta canlı sahne snapshot'tan geri yüklenir (move semantiği ile, ek kopya olmadan).
 */

#include "scene.h"
#include "../Systems/camera_system.h"

struct EngineContext;

/**
 * @class scene_session
 * @brief Oynatma/Durdurma modları arasında sahne yaşam döngüsünü ve geri yükleme (snapshot) durumunu yönetir.
 */
class scene_session
{
public:
    scene_session() = default;
    ~scene_session() = default;

    scene_session(const scene_session&) = delete;
    scene_session& operator=(const scene_session&) = delete;

    /**
     * @brief Editör panellerinin (Hierarchy, Inspector, Gizmo) düzenlediği canlı sahneye referans döner.
     * @note PLAY modunda da aynı canlı sahneyi döndürür; editör oyun sırasında dünyayı canlı inceleyebilir.
     */
    scene& editor_scene() noexcept;
    const scene& editor_scene() const noexcept;

    /**
     * @brief Oyun mantığının, fiziğin ve Game View'ın çalıştığı aktif sahneyi döner.
     * @note Tek canlı dünya mimarisinde editor_scene() ile aynı nesnedir.
     */
    scene& game_scene() noexcept;
    const scene& game_scene() const noexcept;

    /**
     * @brief Oyunun şu anda oynatma modunda olup olmadığını kontrol eder.
     */
    [[nodiscard]] bool is_playing() const noexcept;

    /**
     * @brief Editör modundan çıkıp oyun simülasyonunu başlatır.
     *
     * Canlı sahnenin tek seferlik yedeğini alır ve native script'leri ilklendirir.
     *
     * @param ctx Motor bağlamı (Script ilklendirmeleri için gereklidir).
     * @param takeSnapshot STOP'ta geri dönüş için yedek alınıp alınmayacağı (Standalone build'de gereksizdir).
     */
    void play(EngineContext& ctx, bool takeSnapshot = true);

    /**
     * @brief Oyun simülasyonunu durdurur, script örneklerini yok eder ve sahneyi yedekten geri yükler.
     * @note Fizik dünyası bu çağrıdan ÖNCE yok edilmelidir (rigidbody body_id'leri canlı sahnededir).
     */
    void stop();

private:
    /**
     * @brief Verilen sahnenin tam bir kopyasını (bileşenleri ve varlıklarıyla birlikte) oluşturur.
     * @note Yalnızca PLAY başlangıcında bir kez çağrılır; kare başına çağrılmamalıdır.
     */
    [[nodiscard]] static scene clone_scene(const scene& src);

    scene m_liveScene;          ///< Editör ve oyunun paylaştığı tek canlı sahne.
    scene m_playSnapshot;       ///< PLAY'e basılmadan hemen önce alınan geri yükleme yedeği.
    bool m_hasSnapshot = false; ///< Geçerli bir yedek alınıp alınmadığı.
    bool m_playing = false;     ///< Oynatma durum bayrağı.
};