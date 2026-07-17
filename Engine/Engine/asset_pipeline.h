#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../Engine/Assets/aria.h"

class EngineContext;

namespace asset_pipeline
{
    aria::asset_guid get_or_create_meta_guid(const std::string& asset_path);

    // 📥 Dışarıdan gelen gerçek dosyaları Aria'ya kaydeder (Rastgele UUID v4 üretir)
    aria::handle<aria::model_asset>    import_model(aria::system* aria_system, const std::string& path);
    aria::handle<aria::texture_asset>  import_texture(EngineContext& ctx, const std::string& path);
    aria::handle<aria::material_asset> import_material(aria::system* aria_system, const std::string& path);
    aria::handle<aria::shader_asset>   import_shader(EngineContext& ctx, const std::string& path);

    // 🛠️ Sanal Yol (Virtual Path) kullanarak Aria üzerinden primitifleri üretir
    aria::handle<aria::model_asset>   create_cube_mesh(aria::system* aria_system);
    aria::handle<aria::model_asset>   create_sphere_mesh(aria::system* aria_system);


}