#pragma once

/**
 * @file top_bar_view_model.h
 * @brief Editörün üst kontrol çubuğu (Top Bar) için Model-View-ViewModel (MVVM) yapısının görünüm modeli katmanını tanımlar.
 */

#include <string>
#include <functional>
#include "../../Editor/editor_context.h"

namespace vesta
{
    /**
     * @class top_bar_view_model
     * @brief Editör modları (Oyun/Editör) arası geçişleri ve sahne dosya işlemlerini (Kaydet/Yükle) yönetir.
     */
    class top_bar_view_model
    {
    public:
        /**
         * @brief Dosya yolu gerektiren eylemler (Kaydet/Yükle) için genel geri çağrım (callback) tanımı.
         */
        using FileActionCallback = std::function<void(const std::string&)>;

        /**
         * @brief Üst menü görünüm modelini başlatır.
         *
         * @param editorCtx Global editör durumu ve bağlamı.
         * @param onSaveRequested Sahne kaydetme işlemi tetiklendiğinde çağrılacak fonksiyon.
         * @param onLoadRequested Sahne yükleme işlemi tetiklendiğinde çağrılacak fonksiyon.
         */
        top_bar_view_model(
            EditorContext& editorCtx,
            FileActionCallback onSaveRequested,
            FileActionCallback onLoadRequested)
            : m_editor_ctx(editorCtx)
            , m_onSaveRequested(std::move(onSaveRequested))
            , m_onLoadRequested(std::move(onLoadRequested))
        {
        }

        /**
         * @brief Motorun mevcut çalışma modunu (Oyun veya Editör) döndürür.
         */
        EngineMode get_mode() const
        {
            return m_editor_ctx.mode;
        }

        /**
         * @brief Oyun simülasyonunun aktif (Play veya Paused) olup olmadığını döndürür.
         */
        bool is_playing() const
        {
            return m_editor_ctx.mode == EngineMode::PLAY;
        }

        /**
         * @brief Oyun simülasyonunun duraklatılmış (Paused) olup olmadığını döndürür.
         */
        bool is_paused() const
        {
            return m_editor_ctx.mode == EngineMode::PLAY && m_editor_ctx.isPaused;
        }

        /**
         * @brief Oyun simülasyonunu başlatır veya duraklatılmışsa devam ettirir (Resume).
         */
        void play()
        {
            m_editor_ctx.mode = EngineMode::PLAY;
            m_editor_ctx.isPaused = false;
        }

        /**
         * @brief Aktif oyun simülasyonunu duraklatır veya devam ettirir (Pause / Resume).
         */
        void toggle_pause()
        {
            if (m_editor_ctx.mode == EngineMode::PLAY)
            {
                m_editor_ctx.isPaused = !m_editor_ctx.isPaused;
            }
        }

        /**
         * @brief Oyun simülasyonunu tamamen durdurur ve sahneyi başlangıç durumuna (Snapshot) döndürür.
         */
        void stop()
        {
            m_editor_ctx.mode = EngineMode::EDITOR;
            m_editor_ctx.isPaused = false;
        }

        /**
         * @brief Motor çalışma modunu Oyun (Play) ile Editör (Edit) arasında değiştirir (Toggle).
         */
        void toggle_mode()
        {
            if (m_editor_ctx.mode == EngineMode::EDITOR)
            {
                play();
            }
            else
            {
                stop();
            }
        }

        /**
         * @brief Belirtilen dosya yolu için kayıt işlemini başlatmak üzere bağlı callback'i tetikler.
         * @param path Sahnenin kaydedileceği hedef dosya yolu.
         */
        void execute_save(const std::string& path)
        {
            if (m_onSaveRequested)
            {
                m_onSaveRequested(path);
            }
        }

        /**
         * @brief Belirtilen dosya yolundan yükleme işlemini başlatmak üzere bağlı callback'i tetikler.
         * @param path Sahnenin yükleneceği dosya yolu.
         */
        void execute_load(const std::string& path)
        {
            if (m_onLoadRequested)
            {
                m_onLoadRequested(path);
            }
        }

    private:
        EditorContext& m_editor_ctx;            ///< Motorun genel editör bağlam referansı.

        FileActionCallback m_onSaveRequested;   ///< Kaydetme isteğini dışarı ileten fonksiyon.
        FileActionCallback m_onLoadRequested;   ///< Yükleme isteğini dışarı ileten fonksiyon.
    };

} // namespace vesta