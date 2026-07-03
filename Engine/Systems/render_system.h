#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "../Renderer/render_queue.h"
#include "../Systems/light_system.h"

class registry;
struct EngineContext;
struct render_item;
struct light_batch;

class render_system
{
public:
    render_system();
    ~render_system();

    void init();
    // 🗑️ bind_light_buffer(shader& s) UÇURULDU! UBO artık global binding noktası (0) üzerinden doğrudan bağlı.

    void render(
        const registry& reg,
        const light_batch& lightBatch,
        struct EngineContext& ctx,
        const glm::mat4& view,
        const glm::mat4& projection
    );

private:
    unsigned int m_LightUBO = 0;
    std::vector<render_item> m_RenderItemsCache;
    render_queue m_RenderQueueCache;

    void extract(
        const registry& reg,
        struct EngineContext& ctx,
        const glm::mat4& viewProj
    );

    glm::mat4 calculate_light_space_matrix(const light_data& light, const glm::vec3& viewPos);
};