#include <doctest/doctest.h>
#include "../Engine/Renderer/shadow_map_fbo.h"
#include "../Engine/Renderer/render_settings.h"
#include "../Engine/Systems/render_system.h"
#include "../Engine/Engine/engine_context.h"
#include <glm/gtc/matrix_transform.hpp>

TEST_SUITE("CascadedShadowMaps")
{
    TEST_CASE("CSM: Render Settings and Cascade Splits")
    {
        render_settings settings;
        CHECK(settings.enable_csm == true);
        CHECK(settings.csm_cascade_count == 3);
        CHECK(settings.csm_split_distances[0] == doctest::Approx(15.0f));
        CHECK(settings.csm_split_distances[1] == doctest::Approx(50.0f));
        CHECK(settings.csm_split_distances[2] == doctest::Approx(150.0f));
        CHECK(settings.csm_split_distances[0] < settings.csm_split_distances[1]);
        CHECK(settings.csm_split_distances[1] < settings.csm_split_distances[2]);
    }

    TEST_CASE("CSM: Frustum Splitting and Light Space Matrix Generation")
    {
        EngineContext ctx;
        ctx.renderSettings.enable_csm = true;
        ctx.renderSettings.shadow_map_resolution = 2048;

        render_system renderer;

        light_data sun;
        sun.type = 0; // Directional
        sun.direction = glm::vec3(-0.5f, -1.0f, -0.3f);
        sun.color = glm::vec3(1.0f);
        sun.intensity = 1.0f;

        const glm::vec3 cameraPos(0.0f, 2.0f, 10.0f);
        const glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);

        auto cascades = renderer.calculate_csm_cascades(ctx, sun, view, proj, cameraPos);

        CHECK(cascades.size() == 3);

        // Verify split distances
        CHECK(cascades[0].split_distance == doctest::Approx(15.0f));
        CHECK(cascades[1].split_distance == doctest::Approx(50.0f));
        CHECK(cascades[2].split_distance == doctest::Approx(150.0f));

        // Verify that light space matrices are valid (invertible / non-zero determinant)
        for (size_t i = 0; i < cascades.size(); ++i)
        {
            const float det = glm::determinant(cascades[i].light_space_matrix);
            CHECK(std::abs(det) > 1e-9f);

            // Test transformation of a point right in front of the camera
            const glm::vec4 testPointWorld(0.0f, 1.0f, 0.0f, 1.0f);
            const glm::vec4 ptLightSpace = cascades[i].light_space_matrix * testPointWorld;
            CHECK(std::abs(ptLightSpace.w) == doctest::Approx(1.0f)); // Ortho projection preserves w = 1
            CHECK(ptLightSpace.z >= -1.0f);
            CHECK(ptLightSpace.z <= 1.0f);
        }
    }

    TEST_CASE("CSM: Shadow Map FBO Cascade Management")
    {
        shadow_map_fbo fbo;
        CHECK(fbo.get_cascade_count() == 3);
        CHECK(fbo.get_depth_texture() == 0);
        CHECK(fbo.get_depth_texture_array() == 0);
        fbo.destroy();
    }
}

