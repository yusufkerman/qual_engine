#pragma once

/**
 * @file render_system.h
 * @brief Sahne varlıklarını GPU'ya aktaran, gruplayan (batching) ve çizen ana render boru hattı (pipeline).
 */

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "../Renderer/render_queue.h"
#include "../Systems/light_system.h"

class registry;
struct EngineContext;
struct render_item;
struct light_batch;

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

private:
    unsigned int m_LightUBO;                            ///< Tüm ışık verilerini shader'lara taşıyan Uniform Buffer Object kimliği.
    std::vector<render_item> m_RenderItemsCache;        ///< Çıkarım aşamasında toplanan render paketlerinin geçici önbelleği.
    render_queue m_RenderQueueCache;                    ///< Shader ID'lerine göre gruplanmış çizim kuyruğu.
    render_pass_stats m_SceneStats{};                   ///< Sahne görünümü (Scene Pass) çizim telemetrisi.
    render_pass_stats m_GameStats{};                    ///< Oyun görünümü (Game Pass) çizim telemetrisi.

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
     * @param light Matrisi hesaplanacak ışık verisi.
     * @param viewPos Kameranın dünya uzayındaki konumu.
     * @return Işık uzayı (Light Space) dönüşüm matrisi.
     */
    glm::mat4 calculate_light_space_matrix(const light_data& light, const glm::vec3& viewPos);
};