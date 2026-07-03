#pragma once
#include <string>

struct EditorContext;

namespace aria::editor {

    /**
     * @brief Farklı varlık tiplerinin (Material, Prefab vb.) Inspector üzerinde
     * çizilmesini sağlayan modüler arayüz sınıfı.
     */
    class IAssetInspector {
    public:
        virtual ~IAssetInspector() = default;

        /**
         * @brief Asset seçildiğinde Inspector panelinin içerik çizim sorumluluğunu üstlenir.
         * @param asset_path Seçili asset dosyasının tam disk yolu.
         * @param editorCtx Editör çalışma zamanı bağlamı.
         */
        virtual void on_render_inspector(const std::string& asset_path, EditorContext& editorCtx) = 0;
    };

} // namespace aria::editor