#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../Renderer/render_queue.h"
#include "../Systems/light_system.h"

class registry;
struct EngineContext;
struct render_item;
struct light_batch;
class shader;

/**
 * @brief Sahnedeki render işlemlerini yöneten ana sistem.
 * * Mesh toplama (extract), frustum culling, material çözümleme
 * ve shader batching işlemlerini gerçekleştirir.
 */
class render_system
{
public:
    render_system();
    ~render_system();

    void init();
    void bind_light_buffer(shader& s);

    /**
     * @brief Sahneyi render eder.
     * * @param reg ECS registry (entity-component erişimi)
     * @param ctx Engine context (shader, renderer, asset manager)
     * @param view Kamera view matrisi
     * @param projection Kamera projection matrisi
     */
    void render(
        const registry& reg,
        const light_batch& lightBatch,
        struct EngineContext& ctx,
        const glm::mat4& view,
        const glm::mat4& projection
    );

private:
    /// Işık verileri için GPU taraflı Uniform Buffer Object
    unsigned int m_LightUBO = 0;

    /// Frame başına render edilecek mesh listesi (allocation azaltmak için cache)
    std::vector<render_item> m_RenderItemsCache;

    /// Shader bazlı batch render yapısı
    render_queue m_RenderQueueCache;

    /**
     * @brief Sahnedeki render edilebilir objeleri toplar.
     * * Frustum culling + material çözümleme burada yapılır.
     */
    void extract(
        const registry& reg,
        struct EngineContext& ctx,
        const glm::mat4& viewProj
    );

    glm::mat4 calculate_light_space_matrix(const light_data& light, const glm::vec3& viewPos);
};