#pragma once

/**
 * @file render_system.h
 * @brief Sahne varlıklarını GPU'ya aktaran, gruplayan (batching) ve çizen ana render boru hattı (pipeline).
 */

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include "../Renderer/render_queue.h"
#include "../Systems/light_system.h"

class registry;
struct EngineContext;
struct render_item;
struct light_batch;

/**
 * @struct shader_uniform_cache
 * @brief Bir GPU shader programına ait önbelleklenmiş uniform lokasyonları (Sıfır glGetUniformLocation string araması).
 */
struct shader_uniform_cache
{
    // Global pass üniformaları
    int u_view = -1;
    int u_projection = -1;
    int u_viewPos = -1;
    int u_lightSpaceMatrix = -1;
    int u_FogEnabled = -1;
    int u_FogColor = -1;
    int u_FogDensity = -1;
    int u_shadowMap = -1;

    // Cascaded Shadow Maps (CSM)
    int u_EnableCSM = -1;
    int u_ShadowMapArray = -1;
    int u_CascadeCount = -1;
    int u_CascadePlaneDistances = -1;
    int u_CascadeLightSpaceMatrices = -1;

    // Nesne bazlı (Per-Object) üniformalar
    int u_model = -1;
    int u_baseColor = -1;
    int u_TextureScale = -1;
    int u_Metallic = -1;
    int u_Roughness = -1;
    int u_AO = -1;
    int u_EmissiveColor = -1;
    int u_EmissiveIntensity = -1;
    int u_useTexture = -1;
    int u_diffuseTex = -1;
    int u_useNormalMap = -1;
    int u_normalTex = -1;
    int u_normalStrength = -1;
    int u_UseMetallicMap = -1;
    int u_metallicTex = -1;
    int u_UseRoughnessMap = -1;
    int u_roughnessTex = -1;
    int u_UseAOMap = -1;
    int u_aoTex = -1;
    int u_UseEmissiveMap = -1;
    int u_emissiveTex = -1;
    int u_specularStrength = -1;
    int u_shininess = -1;
    int u_RenderMode = -1;
    int u_AlphaThreshold = -1;

    // Gökyüzü Çevre Yansıması (Skybox IBL Reflection)
    int u_UseSkyboxReflection = -1;
    int u_SkyboxTex = -1;

    // PS1 Retro Horror Parametreleri
    int u_PS1VertexSnapping = -1;
    int u_PS1AffineMapping = -1;
    int u_PS1ColorDepth = -1;
    int u_PS1Dithering = -1;
    int u_PS1DitherStrength = -1;

    // Standart PBR Küresel Işıklandırma Parametreleri
    int u_PbrAmbientIntensity = -1;
    int u_PbrIBLIntensity = -1;
    int u_PbrSpecularIntensity = -1;
    int u_PbrRoughnessMultiplier = -1;
    int u_PbrMetallicMultiplier = -1;
    int u_PbrExposure = -1;
    int u_PbrToneMapping = -1;

    // Dolaylı Işık (GI) ve Gelişmiş Atmosferik Sis Parametreleri
    int u_PS1Mode = -1;
    int u_EnableGI = -1;
    int u_GISkyColor = -1;
    int u_GIGroundColor = -1;
    int u_FogMode = -1;
    int u_FogHeightFalloff = -1;
    int u_FogBaseHeight = -1;
    int u_FogSunScattering = -1;
    int u_FogLinkAmbient = -1;
    int u_VolumetricActive = -1;
    int u_VolumetricMaxDistance = -1;

    // Spot ve Point Isik Golgeleri
    int u_SpotLightSpaceMatrix = -1;
    int u_SpotShadowMap = -1;
    int u_HasSpotShadow = -1;
    int u_PointShadowMap = -1;
    int u_HasPointShadow = -1;
    int u_PointShadowLightPos = -1;
    int u_PointShadowFarPlane = -1;
};

/**
 * @struct shadow_uniform_cache
 * @brief Gölge geçişi shader'larına ait önbelleklenmiş uniform lokasyonları.
 */
struct shadow_uniform_cache
{
    int u_lightSpaceMatrix = -1;
    int u_model = -1;
};

/**
 * @struct csm_cascade_data
 * @brief Kaskatli golge haritalama dilimi icin isik uzayi matrisi ve kesme mesafesi.
 */
struct csm_cascade_data
{
    glm::mat4 light_space_matrix{ 1.0f };
    float split_distance = 0.0f;
};

/**
 * @struct render_pass_stats
 * @brief Bir çizim geçişine (Scene veya Game) ait anlık GPU/CPU istatistiklerini ve optimizasyon metriklerini tutar.
 */
struct render_pass_stats
{
    uint32_t draw_calls_main = 0;       ///< Ana çizim geçişindeki glDrawElements çağrı sayısı.
    uint32_t draw_calls_shadow = 0;     ///< Gölge geçişindeki glDrawElements çağrı sayısı.
    uint32_t total_draw_calls = 0;      ///< Toplam çizim çağrısı (Main + Shadow).
    uint32_t triangles_main = 0;        ///< Ana geçişte çizilen üçgen (triangle) sayısı.
    uint32_t triangles_shadow = 0;      ///< Gölge geçişinde çizilen üçgen sayısı.
    uint32_t visible_submeshes = 0;     ///< Frustum Culling sonrası ekranda kalan alt ağ (submesh) sayısı.
    uint32_t culled_submeshes = 0;      ///< Frustum Culling tarafından elenen alt ağ sayısı.
    uint32_t shader_batches = 0;        ///< Aktif benzersiz GPU shader grubu (batch) sayısı.
    uint32_t active_lights = 0;         ///< UBO'ya yüklenen aktif ışık sayısı.
    bool shadow_pass_active = false;    ///< Yönlü ışık gölge geçişinin aktif olup olmadığı.
};

/**
 * @class render_system
 * @brief Görüş alanı kırpma (Frustum Culling), gölge haritalama ve malzeme tabanlı çizim işlemlerini koordine eden sistem.
 */
class render_system
{
public:
    render_system();
    ~render_system();

    /**
     * @brief Render sisteminin bağımlı olduğu GPU kaynaklarını (Örn: Işık UBO'su) oluşturur.
     * @note Bu fonksiyon GLAD (OpenGL) bağlamı ilklendirilmeden çağrılmamalıdır.
     */
    void init();

    /**
     * @brief Sahneyi ana ekrana ve gölge haritalarına render eder.
     *
     * Pipeline akışı:
     * 1. Işık verisinin UBO aracılığıyla GPU'ya iletilmesi.
     * 2. ECS'den render edilebilir nesnelerin çıkarılması (Extraction & Culling).
     * 3. Shader bazlı çizim gruplaması (Batching).
     * 4. Gölge geçişi (Shadow Pass) ve ana çizim geçişi (Main Pass).
     *
     * @param reg Sahnedeki varlık kayıt defteri.
     * @param lightBatch GPU'ya aktarılacak paketlenmiş ışık verileri.
     * @param ctx Motor bağlamı (FBO, Renderer ve Aria varlık sistemlerine erişim).
     * @param view Kameranın görünüm (View) matrisi.
     * @param projection Kameranın iz düşüm (Projection) matrisi.
     * @param isSceneViewport Çizimin editör sahne görünümü için mi yoksa oyun görünümü için mi yapıldığını belirtir.
     */
    void render(
        const registry& reg,
        const light_batch& lightBatch,
        struct EngineContext& ctx,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool isSceneViewport = false
    );

    /**
     * @brief Sahne görünümüne (Scene Viewport) ait son karesel çizim istatistiklerini döndürür.
     */
    [[nodiscard]] const render_pass_stats& scene_stats() const noexcept { return m_SceneStats; }

    /**
     * @brief Oyun görünümüne (Game Viewport) ait son karesel çizim istatistiklerini döndürür.
     */
    [[nodiscard]] const render_pass_stats& game_stats() const noexcept { return m_GameStats; }

    /**
     * @brief Shader uniform lokasyon önbelleğini sıfırlar (Shader hot-reload veya yeniden derleme için).
     */
    void clear_uniform_cache() noexcept;

    /**
     * @brief Kaskatli golge haritasi dilimlerini (CSM) hesaplar.
     */
    std::vector<csm_cascade_data> calculate_csm_cascades(
        const struct EngineContext& ctx,
        const light_data& light,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition
    );

    /**
     * @brief Sahnedeki aktif yönlü ışığın (Directional Light) varlığını döner.
     */
    [[nodiscard]] bool has_directional_light() const noexcept { return m_HasDirectionalLight; }

    /**
     * @brief Sahnedeki aktif yönlü ışığın verilerini döner.
     */
    [[nodiscard]] const light_data& get_directional_light() const noexcept { return m_DirectionalLight; }

    /**
     * @brief Yönlü ışığın gölge projeksiyon matrisini döner (Tekli harita için).
     */
    [[nodiscard]] const glm::mat4& get_light_space_matrix() const noexcept { return m_DirectionalLightSpaceMatrix; }

    /**
     * @brief Kaskatlı gölge dilimlerini (CSM Cascades) döner.
     */
    [[nodiscard]] const std::vector<csm_cascade_data>& get_csm_cascades() const noexcept { return m_CSMCascades; }

private:
    bool m_HasDirectionalLight = false;                 ///< Son çizim karesinde aktif yönlü ışık bulundu mu?
    light_data m_DirectionalLight{};                    ///< Aktif yönlü ışık verisi.
    glm::mat4 m_DirectionalLightSpaceMatrix{ 1.0f };    ///< Yönlü ışık gölge uzayı matrisi.
    std::vector<csm_cascade_data> m_CSMCascades;        ///< Kaskatlı gölge haritası dilimleri.
    unsigned int m_LightUBO;                            ///< Tüm ışık verilerini shader'lara taşıyan Uniform Buffer Object kimliği.
    std::vector<render_item> m_RenderItemsCache;        ///< Çıkarım aşamasında toplanan render paketlerinin geçici önbelleği.
    render_queue m_RenderQueueCache;                    ///< Shader ID'lerine göre gruplanmış çizim kuyruğu.
    std::vector<render_item> m_CascadeShadowItems[3];   ///< Kaskatlı gölge haritaları için gölge düşüren (caster) nesneler.
    std::vector<render_item> m_SingleShadowItems;       ///< Tekli gölge haritası için gölge düşüren nesneler.
    std::vector<render_item> m_SpotShadowItems;         ///< Spot isik icin golge dusuren nesneler.
    std::vector<render_item> m_PointShadowItems;        ///< Noktasal isik icin golge dusuren nesneler.

    bool m_HasSpotLightShadow = false;
    light_data m_ShadowSpotLight{};
    glm::mat4 m_SpotLightSpaceMatrix{ 1.0f };

    bool m_HasPointLightShadow = false;
    light_data m_ShadowPointLight{};
    float m_PointShadowFarPlane = 10.0f;
    render_pass_stats m_SceneStats{};                   ///< Sahne görünümü (Scene Pass) çizim telemetrisi.
    render_pass_stats m_GameStats{};                    ///< Oyun görünümü (Game Pass) çizim telemetrisi.

    std::unordered_map<unsigned int, shader_uniform_cache> m_ShaderUniformCache;
    std::unordered_map<unsigned int, shadow_uniform_cache> m_ShadowUniformCache;

    const shader_uniform_cache& get_shader_uniforms(unsigned int shaderID);
    const shadow_uniform_cache& get_shadow_uniforms(unsigned int shaderID);

    /**
     * @brief ECS üzerinden görünür varlıkları seçer ve render önbelleğine alır.
     * @param reg Varlık kayıt defteri.
     * @param ctx Motor bağlamı (Aria varlık sorguları için).
     * @param viewProj Görüş alanı kırpması (Frustum Culling) için View * Projection matrisi.
     * @param outStats Görünür ve kırpılan alt ağ sayaçlarının yazılacağı istatistik yapısı.
     */
    void extract(
        const registry& reg,
        struct EngineContext& ctx,
        const glm::mat4& viewProj,
        render_pass_stats& outStats
    );

    /**
     * @brief Yönlü ışıklar (Directional) için gölge izdüşüm ve görünüm matrisini hesaplar.
     * @param ctx Motor bağlamı (Gölge projeksiyon ayarları için).
     * @param light Matrisi hesaplanacak ışık verisi.
     * @param viewPos Kameranın dünya uzayındaki konumu.
     * @return Işık uzayı (Light Space) dönüşüm matrisi.
     */
    glm::mat4 calculate_light_space_matrix(const struct EngineContext& ctx, const light_data& light, const glm::vec3& viewPos);
    glm::mat4 calculate_spot_light_space_matrix(const light_data& light);

};
