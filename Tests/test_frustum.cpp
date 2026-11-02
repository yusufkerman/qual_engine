#include <doctest/doctest.h>
#include "Renderer/frustum.h"
#include <glm/gtc/matrix_transform.hpp>

TEST_CASE("Frustum - Plane Extraction and Normalization") {
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewProj = proj * view;

    frustum f = build_frustum(viewProj);

    SUBCASE("All 6 planes have normalized normals (unit length)") {
        for (const auto& p : f.planes) {
            float len = glm::length(p.normal);
            CHECK(len == doctest::Approx(1.0f).epsilon(0.001f));
        }
    }
}

TEST_CASE("Frustum - Bounding Sphere Visibility (is_inside)") {
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 100.0f);
    // Camera at origin looking down -Z, Y up
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 viewProj = proj * view;

    frustum f = build_frustum(viewProj);

    SUBCASE("Point in front of camera is visible") {
        glm::vec3 center(0.0f, 0.0f, -10.0f);
        float radius = 1.0f;
        CHECK(f.is_inside(center, radius));
    }

    SUBCASE("Point behind camera is culled") {
        glm::vec3 center(0.0f, 0.0f, 10.0f); // Positive Z is behind
        float radius = 1.0f;
        CHECK_FALSE(f.is_inside(center, radius));
    }

    SUBCASE("Point beyond far plane is culled") {
        glm::vec3 center(0.0f, 0.0f, -150.0f); // Far plane is at 100
        float radius = 5.0f;
        CHECK_FALSE(f.is_inside(center, radius));
    }

    SUBCASE("Point way off to the side is culled") {
        glm::vec3 center(200.0f, 0.0f, -10.0f);
        float radius = 2.0f;
        CHECK_FALSE(f.is_inside(center, radius));
    }

    SUBCASE("Large sphere touching frustum boundary is preserved (not culled)") {
        glm::vec3 center(15.0f, 0.0f, -10.0f);
        float radius = 20.0f; // Large enough to intersect side plane
        CHECK(f.is_inside(center, radius));
    }
}

