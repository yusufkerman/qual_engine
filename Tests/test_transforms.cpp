#include <doctest/doctest.h>
#include "ECS/registry.h"
#include "ECS/components.h"
#include "ECS/transform_utils.h"

TEST_CASE("Transform Utils - Local Matrix Computation") {
    transform_component t;
    t.position = glm::vec3(10.0f, -5.0f, 2.5f);
    t.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    t.scale = glm::vec3(2.0f, 3.0f, 4.0f);

    glm::mat4 m = ecs_transform::compute_local_matrix(&t);

    // Translation vector in matrix is col 3: [3][0], [3][1], [3][2]
    CHECK(m[3][0] == doctest::Approx(10.0f));
    CHECK(m[3][1] == doctest::Approx(-5.0f));
    CHECK(m[3][2] == doctest::Approx(2.5f));

    // Scale components along diagonal
    CHECK(m[0][0] == doctest::Approx(2.0f));
    CHECK(m[1][1] == doctest::Approx(3.0f));
    CHECK(m[2][2] == doctest::Approx(4.0f));
}

TEST_CASE("Transform Utils - Hierarchy World Matrix Traversal") {
    registry reg;

    // Parent entity at (10, 0, 0)
    entity parent = reg.create_entity();
    reg.add_component<transform_component>(parent, transform_component{ glm::vec3(10.0f, 0.0f, 0.0f) });
    reg.add_component<hierarchy_component>(parent, hierarchy_component{ 0, 0, 0, 0 });

    // Child entity at (5, 2, 0), parented to 'parent'
    entity child = reg.create_entity();
    reg.add_component<transform_component>(child, transform_component{ glm::vec3(5.0f, 2.0f, 0.0f) });
    reg.add_component<hierarchy_component>(child, hierarchy_component{ parent, 0, 0, 0 });

    // Grandchild entity at (0, 0, 3), parented to 'child'
    entity grandchild = reg.create_entity();
    reg.add_component<transform_component>(grandchild, transform_component{ glm::vec3(0.0f, 0.0f, 3.0f) });
    reg.add_component<hierarchy_component>(grandchild, hierarchy_component{ child, 0, 0, 0 });

    glm::mat4 pMat = ecs_transform::get_world_matrix(parent, reg);
    glm::mat4 cMat = ecs_transform::get_world_matrix(child, reg);
    glm::mat4 gMat = ecs_transform::get_world_matrix(grandchild, reg);

    // Parent world pos = (10, 0, 0)
    CHECK(pMat[3][0] == doctest::Approx(10.0f));
    CHECK(pMat[3][1] == doctest::Approx(0.0f));
    CHECK(pMat[3][2] == doctest::Approx(0.0f));

    // Child world pos = 10 + 5 = 15, y = 2
    CHECK(cMat[3][0] == doctest::Approx(15.0f));
    CHECK(cMat[3][1] == doctest::Approx(2.0f));
    CHECK(cMat[3][2] == doctest::Approx(0.0f));

    // Grandchild world pos = (15, 2, 3)
    CHECK(gMat[3][0] == doctest::Approx(15.0f));
    CHECK(gMat[3][1] == doctest::Approx(2.0f));
    CHECK(gMat[3][2] == doctest::Approx(3.0f));
}

TEST_CASE("Transform Utils - Matrix Decomposition (apply_world_matrix_as_local)") {
    glm::mat4 world(1.0f);
    world = glm::translate(world, glm::vec3(12.0f, -8.0f, 4.0f));
    world = glm::scale(world, glm::vec3(2.5f, 0.5f, 1.0f));

    transform_component target;
    ecs_transform::apply_world_matrix_as_local(target, world);

    CHECK(target.position.x == doctest::Approx(12.0f));
    CHECK(target.position.y == doctest::Approx(-8.0f));
    CHECK(target.position.z == doctest::Approx(4.0f));

    CHECK(target.scale.x == doctest::Approx(2.5f));
    CHECK(target.scale.y == doctest::Approx(0.5f));
    CHECK(target.scale.z == doctest::Approx(1.0f));
}

