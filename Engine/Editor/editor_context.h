#pragma once

/**
 * @file editor_context.h
 * @brief Editör alt sistemleri ve paneller arasında paylaşılan çalışma zamanı bağlamını tanımlar.
 */

#include <glm/glm.hpp>
#include <string>
#include "../Editor/engine_mode.h"

class EditorCamera;
class ui_manager;
class RenderTarget;

/**
 * @enum FocusedPanel
 * @brief Editör içerisinde anlık olarak klavye veya fare odağına sahip panelleri listeler.
 */
enum class FocusedPanel {
    None,
    SceneViewport,
    GameViewport,
    Inspector,
    Hierarchy,
    AssetBrowser
};

/**
 * @struct EditorContext
 * @brief Editörün genel kamera referansını, render hedeflerini, modunu ve geçici panolarını taşıyan bağlam yapısı.
 */
struct EditorContext
{
    EditorCamera* camera = nullptr;             ///< Aktif editör serbest uçuş kamerası.
    ui_manager* ui = nullptr;                   ///< Kullanıcı arayüzü yöneticisi işaretçisi.

    RenderTarget* sceneTarget = nullptr;        ///< Sahne görünümü FBO render hedefi.
    RenderTarget* gameTarget = nullptr;         ///< Oyun görünümü FBO render hedefi.

    FocusedPanel focusedPanel = FocusedPanel::None; ///< O an odaklanılmış editör paneli.

    unsigned int sceneTexture = 0;              ///< Sahne FBO renk dokusu kimliği.
    unsigned int gameTexture = 0;               ///< Oyun FBO renk dokusu kimliği.

    EngineMode mode = EngineMode::EDITOR;       ///< Motorun geçerli modu (Editor / Play).
    bool isPaused = false;                      ///< Oyun modundayken (Play) simülasyonun duraklatılıp duraklatılmadığını belirtir.

    int clipboardEntity = -1;                   ///< Kopyalama/Yapıştırma (Copy-Paste) için panodaki varlık kimliği.
    std::string selectedAssetPath;              ///< Varlık tarayıcısında anlık seçili dosya yolu.

    /**
     * @brief Sahne viewport doku kimliğini günceller.
     */
    void setSceneTexture(unsigned int tex) { sceneTexture = tex; }

    /**
     * @brief Oyun viewport doku kimliğini günceller.
     */
    void setGameTexture(unsigned int tex) { gameTexture = tex; }
};