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
    aria::handle<aria::model_asset>    import_model(EngineContext& ctx, const std::string& path);
    aria::handle<aria::texture_asset>  import_texture(EngineContext& ctx, const std::string& path);
    aria::handle<aria::material_asset> import_material(EngineContext& ctx, const std::string& path);
    aria::handle<aria::shader_asset>   import_shader(EngineContext& ctx, const std::string& path);

    // 🛠️ Sanal Yol (Virtual Path) kullanarak Aria üzerinden primitifleri üretir
    aria::handle<aria::model_asset>   create_cube_mesh(EngineContext& ctx);
    aria::handle<aria::model_asset>   create_sphere_mesh(EngineContext& ctx, float radius = 1.0f);


}