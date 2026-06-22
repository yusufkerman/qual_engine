#include "asset_pipeline.h"
#include "engine_context.h"
#include "../Importers/ModelLoader.h"
#include <iostream>
#include "../Importers/mesh_generator.h"

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


// Yardımcı Fonksiyon: generator_vertex verilerini senin motorunun iç Vertex formatına dönüştürür.
// ModelLoader.cpp içindeki ComputeBounds yapısını taklit ederek bounds hesaplamasını da el ile yaparız.
static Mesh convert_to_engine_mesh(const generated_mesh_data& generated)
{
    Mesh out;
    out.vertices.reserve(generated.vertices.size());
    out.indices = generated.indices;

    glm::vec3 minB(std::numeric_limits<float>::max());
    glm::vec3 maxB(std::numeric_limits<float>::lowest());

    for (const auto& g_vert : generated.vertices)
    {
        Vertex v;
        // Position
        v.Position[0] = g_vert.position.x;
        v.Position[1] = g_vert.position.y;
        v.Position[2] = g_vert.position.z;

        // Normal
        v.Normal[0] = g_vert.normal.x;
        v.Normal[1] = g_vert.normal.y;
        v.Normal[2] = g_vert.normal.z;

        // TexCoord
        v.TexCoord[0] = g_vert.tex_coords.x;
        v.TexCoord[1] = g_vert.tex_coords.y;

        // Tangent (Şimdilik primitive için boş geçiyoruz, gerekirse sıfırlanabilir)
        v.Tangent[0] = v.Tangent[1] = v.Tangent[2] = 0.0f;

        out.vertices.push_back(v);

        minB = glm::min(minB, g_vert.position);
        maxB = glm::max(maxB, g_vert.position);
    }

    // ModelLoader.cpp içindeki akışa sadık kalıyoruz
    out.localCenter = (minB + maxB) * 0.5f;

    float maxDistSq = 0.0f;
    for (const auto& g_vert : generated.vertices)
    {
        glm::vec3 diff = g_vert.position - out.localCenter;
        maxDistSq = glm::max(maxDistSq, glm::dot(diff, diff));
    }
    out.localRadius = glm::sqrt(maxDistSq);

    return out;
}

imported_model_result asset_pipeline::create_cube_mesh(EngineContext& ctx)
{
    // Sanal bir path belirleyip cache kontrolü yapıyoruz
    std::string internal_path = "internal://primitive_cube";
    if (g_ModelCache.find(internal_path) != g_ModelCache.end())
    {
        return g_ModelCache[internal_path];
    }

    // 1. Ham veriyi üret ve motor formatına çevir
    generated_mesh_data raw_cube = mesh_generator::generate_cube();
    Mesh cube_mesh = convert_to_engine_mesh(raw_cube);

    // 2. Senin motorunun mevcut Model yapısına sar (UploadModel tek parça da olsa Model bekliyor)
    Model model;
    model.meshes.push_back(cube_mesh);

    // 3. GPU'ya yükle ve ID'leri topla
    int baseGPUMeshID = ctx.renderer.UploadModel(model);

    imported_model_result result;
    result.mesh_ids.push_back(static_cast<uint32_t>(baseGPUMeshID));
    result.boundsCenter = cube_mesh.localCenter;
    result.boundsRadius = cube_mesh.localRadius;

    // 4. CPU AssetManager kaydı
    ctx.assetManager.add(cube_mesh);

    // Cache kaydı
    g_ModelCache[internal_path] = result;
    std::cout << "[AssetPipeline] Motor ici Primitve Cube uretildi ve yuklendi." << std::endl;

    return result;
}

imported_model_result asset_pipeline::create_sphere_mesh(EngineContext& ctx, float radius)
{
    // Yarıçapa göre benzersiz bir cache key oluşturuyoruz (Örn: internal://primitive_sphere_1.0)
    std::string internal_path = "internal://primitive_sphere_" + std::to_string(radius);
    if (g_ModelCache.find(internal_path) != g_ModelCache.end())
    {
        return g_ModelCache[internal_path];
    }

    // 1. Ham veriyi üret ve motor formatına çevir
    generated_mesh_data raw_sphere = mesh_generator::generate_sphere(radius, 36, 18);
    Mesh sphere_mesh = convert_to_engine_mesh(raw_sphere);

    // 2. Model yapısına sar
    Model model;
    model.meshes.push_back(sphere_mesh);

    // 3. GPU'ya yükle
    int baseGPUMeshID = ctx.renderer.UploadModel(model);

    imported_model_result result;
    result.mesh_ids.push_back(static_cast<uint32_t>(baseGPUMeshID));
    result.boundsCenter = sphere_mesh.localCenter;
    result.boundsRadius = sphere_mesh.localRadius;

    // 4. CPU AssetManager kaydı
    ctx.assetManager.add(sphere_mesh);

    g_ModelCache[internal_path] = result;
    std::cout << "[AssetPipeline] Motor ici Primitve Sphere uretildi (Radius: " << radius << ")." << std::endl;

    return result;
}
void asset_pipeline::clear_cache()
{
    g_ModelCache.clear();
    g_TextureCache.clear();
}