#pragma once

/**
 * @file asset_ui_registry.h
 * @brief Varlıkların (Asset) Inspector panelinde nasıl çizileceğini belirleyen arayüz kayıt (registry) sistemi.
 */

#include <unordered_map>
#include <functional>
#include <string>

namespace vesta {

    /**
     * @typedef AssetRenderFn
     * @brief Bir varlığın (Asset) Inspector üzerinde ImGui ile çizilmesini sağlayan geri çağrım (callback) fonksiyonu.
     * @param asset_path Çizilecek varlığın dosya yolu.
     */
    using AssetRenderFn = std::function<void(const std::string& asset_path)>;

    /**
     * @typedef AssetCanInspectFn
     * @brief İsteğe bağlı olarak, seçilen dosyanın Inspector'da açılabilir olup olmadığını kontrol eden fonksiyon.
     * @param asset_path Kontrol edilecek varlığın dosya yolu.
     * @return bool Açılabilir (inspectable) ise true, aksi halde false.
     */
    using AssetCanInspectFn = std::function<bool(const std::string& asset_path)>;

    /**
     * @struct AssetUIInfo
     * @brief Bir varlık türü için UI (arayüz) çizim bilgilerini ve geri çağrımlarını (callback) tutar.
     */
    struct AssetUIInfo
    {
        const char* name = "";                  ///< Varlık türünün Inspector'da görünecek başlığı (Örn: "Material").
        AssetRenderFn render_fn;                ///< Varlığı ekrana çizdiren (ImGui) fonksiyon.
        AssetCanInspectFn can_inspect_fn;       ///< Varlığın çizilebilir olup olmadığını kontrol eden fonksiyon.
    };

    /**
     * @class asset_ui_registry
     * @brief Varlık türleri (string) ile onların UI çizim fonksiyonlarını eşleştiren statik merkez.
     * @note Bağımlılıkları (EngineContext vb.) kırmak için tamamen fonksiyonel (callback tabanlı) tasarlanmıştır.
     */
    class asset_ui_registry
    {
    public:
        /**
         * @brief Yeni bir varlık türü için UI çizim mantığını sisteme kaydeder.
         *
         * @param asset_type Varlık türünün metin karşılığı (Örn: "material").
         * @param info Varlığın nasıl çizileceğini barındıran bilgi yapısı.
         */
        static void register_asset(const std::string& asset_type, const AssetUIInfo& info)
        {
            get_assets()[asset_type] = info;
        }

        /**
         * @brief Kayıtlı tüm varlık UI çizici (renderer) haritasını döndürür.
         *
         * @return const std::unordered_map<std::string, AssetUIInfo>& Salt okunur harita referansı.
         */
        static const std::unordered_map<std::string, AssetUIInfo>& get_registry()
        {
            return get_assets();
        }

    private:
        /**
         * @brief Meyer's Singleton deseni ile haritanın tek ve güvenli bir örneğini (instance) oluşturur.
         */
        static std::unordered_map<std::string, AssetUIInfo>& get_assets()
        {
            static std::unordered_map<std::string, AssetUIInfo> assets;
            return assets;
        }
    };

} // namespace vesta