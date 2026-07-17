#pragma once

#include <unordered_map>
#include <functional>
#include <string>

// Forward Declarations - ARTIK BUNA GEREK KALMADI (Silebilirsin)
// class EngineContext; 

namespace vesta {

    /**
     * @brief Bir asset'in Inspector üzerinde çizilmesini sağlayan callback.
     * SAF VESTA: EngineContext bağımlılığı tamamen kaldırıldı!
     */
    using AssetRenderFn = std::function<void(const std::string& asset_path)>;

    /**
     * @brief İsteğe bağlı olarak asset'in açılabilir olup olmadığını kontrol eder.
     */
    using AssetCanInspectFn = std::function<bool(const std::string& asset_path)>;

    /**
     * @brief Asset Inspector meta bilgileri.
     */
    struct AssetUIInfo
    {
        const char* name = "";

        AssetRenderFn render_fn;

        AssetCanInspectFn can_inspect_fn;
    };

    /**
     * @brief Asset Inspector sistemlerinin merkezi kayıt noktası.
     */
    class asset_ui_registry
    {
    public:

        static void register_asset(
            const std::string& asset_type,
            const AssetUIInfo& info)
        {
            get_assets()[asset_type] = info;
        }

        static const std::unordered_map<std::string, AssetUIInfo>& get_registry()
        {
            return get_assets();
        }

    private:

        static std::unordered_map<std::string, AssetUIInfo>& get_assets()
        {
            static std::unordered_map<std::string, AssetUIInfo> assets;
            return assets;
        }
    };

} // namespace vesta