#pragma once

/**
 * @file scene_session.h
 * @brief Editör ve çalışma zamanı (runtime) sahne durumlarını yöneten oturum sınıfı.
 */

#include "scene.h"
#include "../Systems/camera_system.h"

struct EngineContext;

/**
 * @class scene_session
 * @brief Oynatma/Durdurma modları arasında sahne geçişlerini, kopyalanmasını ve durum yönetimini sağlar.
 */
class scene_session
{
public:
    scene_session() = default;
    ~scene_session() = default;

    /**
     * @brief Editör sahnesine referans döner.
     */
    scene& editor_scene();
    const scene& editor_scene() const;

    /**
     * @brief Çalışma zamanı (runtime) sahnesine referans döner.
     */
    scene& runtime_scene();
    const scene& runtime_scene() const;

    /**
     * @brief Mevcut duruma göre (oynatılıyor veya editör) aktif olan sahneyi döner.
     */
    scene& game_scene();
    const scene& game_scene() const;

    /**
     * @brief Oyunun şu anda oynatma modunda olup olmadığını kontrol eder.
     */
    [[nodiscard]] bool is_playing() const;

    /**
     * @brief Editör modundan çıkıp oyun simülasyonunu başlatır. Editör sahnesinin yedeğini alır.
     * @param ctx Motor bağlamı (Script ilklendirmeleri için gereklidir).
     */
    void play(EngineContext& ctx);

    /**
     * @brief Oyun simülasyonunu durdurur ve editör sahnesini yedekten geri yükler.
     */
    void stop();

    /**
     * @brief Çalışma zamanındaki sahne durumunu editör sahnesine kopyalar.
     */
    void sync_editor_from_runtime();

private:
    /**
     * @brief Verilen sahnenin tam bir kopyasını (bileşenleri ve varlıklarıyla birlikte) oluşturur.
     * @param src Kopyalanacak kaynak sahne.
     * @return Kopyalanmış yeni sahne nesnesi.
     */
    scene clone_scene(const scene& src);

    scene editorScene;       ///< Editör üzerinde çalışılan ana sahne.
    scene runtimeScene;      ///< Oyun oynatılırken simüle edilen kopya sahne.
    scene editorSnapshot;    ///< Oynatmaya başlamadan önce alınan editör yedeği.
    bool playing = false;    ///< Oynatma durum bayrağı.
};