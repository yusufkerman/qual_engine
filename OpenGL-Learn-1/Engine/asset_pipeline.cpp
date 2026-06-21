#include "asset_pipeline.h"
#include "engine_context.h"
#include "../Importers/ModelLoader.h"
#include <iostream>

static std::unordered_map<std::string, imported_model_result> g_ModelCache;
static std::unordered_map<std::string, asset_id> g_TextureCache;

imported_model_result asset_pipeline::import_model(EngineContext& ctx, const std::string& path)
{
    // 🧠 1. KONTROL: Bu model dosyası daha önce yüklenmiş mi?
    if (g_ModelCache.find(path) != g_ModelCache.end())
    {
        std::cout << "[AssetPipeline] Model cache'den getirildi: " << path << std::endl;
        return g_ModelCache[path]; // Hafızayı ve GPU'yu şişirmeden eski sonucu dön!
    }

    imported_model_result result;

    // Diskten ham veriyi oku
    Model model = ModelLoader::Load(path);

    if (model.meshes.empty())
        return result;

    // Modeli GPU'ya yüklüyoruz ve MeshRenderer bize base GPU ID'sini dönüyor.
    int baseGPUMeshID = ctx.renderer.UploadModel(model);

    result.mesh_ids.reserve(model.meshes.size());

    // CPU tarafındaki assetManager kaydı
    for (auto& m : model.meshes)
    {
        ctx.assetManager.add(m);
    }

    // mesh_ids içine MeshRenderer'ın ürettiği gerçek GPU mesh ID'lerini dolduruyoruz!
    for (size_t i = 0; i < model.meshes.size(); ++i)
    {
        result.mesh_ids.push_back(baseGPUMeshID + static_cast<uint32_t>(i));
    }

    result.boundsCenter = model.meshes.front().localCenter;
    result.boundsRadius = model.meshes.front().localRadius;

    // 🧠 2. KAYIT: Bir daha çağrılırsa diske gitmesin diye cache tablosuna ekliyoruz
    g_ModelCache[path] = result;

    std::cout << "[AssetPipeline] Model ilk kez yuklendi ve cache'e alindi: " << path << std::endl;

    return result;
}
asset_id asset_pipeline::import_texture(EngineContext& ctx, const std::string& path)
{
    // 🧠 1. Kontrol: Bu dosya yolu daha önce yüklenmiş mi?
    if (g_TextureCache.find(path) != g_TextureCache.end())
    {
        return g_TextureCache[path]; // Varsa direkt texture_manager ID'sini dön
    }

    // 🔄 2. Yükleme: İlk defa geliyorsa senin texture_manager üzerinden yüklüyoruz!
    // NOT: ctx içerisindeki texture_manager değişkeninin adının 'textureManager' olduğunu varsayıyorum.
    unsigned int managerTextureID = ctx.textureManager.load(path);

    if (managerTextureID == 0)
    {
        std::cout << "[AssetPipeline] Texture Manager yukleme basarisiz: " << path << std::endl;
        return 0;
    }

    // 🧠 3. Kayıt: Bir daha basıldığında sıfırdan yüklemesin diye Cache'e ekle
    g_TextureCache[path] = managerTextureID;

    std::cout << "[AssetPipeline] Texture loaded via TextureManager. Path: "
        << path << " -> Manager ID: " << managerTextureID << std::endl;

    return managerTextureID;
}