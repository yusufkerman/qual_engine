#include "asset_pipeline.h"
#include "engine_context.h"
#include "../Importers/ModelLoader.h"

imported_model_result asset_pipeline::import_model(EngineContext& ctx, const std::string& path)
{
    imported_model_result result;

    Model model = ModelLoader::Load(path);

    if (model.meshes.empty())
        return result;

    // 🔥 KRİTİK DÜZELTME: Modeli GPU'ya yüklüyoruz ve MeshRenderer bize base GPU ID'sini dönüyor.
    // Senin yazdığın UploadModel ardışık olarak gpuMeshes dizisine ekleme yaptığı için
    // assetManager'ın ürettiği ID ile gpuMeshes içindeki index'ler birebir eşitlenecektir.
    int baseGPUMeshID = ctx.renderer.UploadModel(model);

    result.mesh_ids.reserve(model.meshes.size());

    // CPU tarafındaki assetManager kaydı (eğer collision vs. için lazımsa kalabilir)
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

    return result;
}