/**
 * @file inspector_panel.h
 * @brief Seçili varlığa (entity) ait bileşenleri veya seçili asset dosyalarını dinamik olarak listeleyen ana editör paneli.
 */
#pragma once
#include "../../Scene/scene.h"
#include <unordered_map>
#include <string>
#include <memory>

 // Forward declarations
struct EngineContext;
struct EditorContext;

namespace aria::editor {
    class IAssetInspector; // Ön bildirim
}

/**
 * @brief Editörün Nesne Özellikleri (Inspector) panel sınıfı. Hem ECS Varlıklarını hem modüler Assetleri yönetir.
 */
class inspector_panel
{
public:
    inspector_panel();
    ~inspector_panel();

    /**
     * @brief Aktif sahne üzerindeki seçili varlığın bileşenlerini VEYA seçili asset'in modüler editörünü çizer.
     */
    void render(scene& scene,
        int selectedEntity,
        EngineContext& engineCtx,
        EditorContext& editorCtx);

private:
    // Kayıtlı alt asset editörlerinin modüler listesi
    std::unordered_map<std::string, std::unique_ptr<aria::editor::IAssetInspector>> m_asset_inspectors;

    // JSON dosyalarının kafasındaki "asset_type" değerini yakalayan güvenli okuyucu
    std::string get_asset_type_from_file(const std::string& path);

};