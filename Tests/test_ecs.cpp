#include "Importers/mesh_generator.h"
#include "Engine/Assets/aria.h"
#include "Engine/asset_pipeline.h"
#include "Resources/engine_resources.h"
#include <doctest/doctest.h>
#include "ECS/registry.h"
#include "ECS/components.h"

TEST_CASE("ECS Registry - Entity Allocation and Reservation") {
    registry reg;

    SUBCASE("Entity 0 is strictly reserved and invalid") {
        CHECK_FALSE(reg.valid(0));
        CHECK(reg.get_component<transform_component>(0) == nullptr);
        CHECK_FALSE(reg.has_component<name_component>(0));
    }

    SUBCASE("Sequential valid entity generation") {
        entity e1 = reg.create_entity();
        entity e2 = reg.create_entity();
        CHECK(e1 == 1);
        CHECK(e2 == 2);
        CHECK(reg.valid(e1));
        CHECK(reg.valid(e2));
        CHECK(reg.get_all_entities().size() == 2);
    }
}

TEST_CASE("ECS Registry - Component CRUD and Multi-Component Views") {
    registry reg;
    entity e1 = reg.create_entity();
    entity e2 = reg.create_entity();

    SUBCASE("Component addition and retrieval") {
        reg.add_component<name_component>(e1, name_component{ "Player" });
        reg.add_component<transform_component>(e1, transform_component{ glm::vec3(10.0f, 20.0f, 30.0f) });

        REQUIRE(reg.has_component<name_component>(e1));
        REQUIRE(reg.has_component<transform_component>(e1));

        auto* nameComp = reg.get_component<name_component>(e1);
        auto* transComp = reg.get_component<transform_component>(e1);

        REQUIRE(nameComp != nullptr);
        REQUIRE(transComp != nullptr);
        CHECK(nameComp->name == "Player");
        CHECK(transComp->position.x == 10.0f);
        CHECK(transComp->position.y == 20.0f);
        CHECK(transComp->position.z == 30.0f);
    }

    SUBCASE("In-place range-for loop modification (auto& [e, comp])") {
        reg.add_component<transform_component>(e1, transform_component{ glm::vec3(5.0f, 0.0f, 0.0f) });
        reg.add_component<transform_component>(e2, transform_component{ glm::vec3(15.0f, 0.0f, 0.0f) });

        for (auto& [ent, trans] : reg.get_all<transform_component>()) {
            trans.position.x += 100.0f;
        }

        CHECK(reg.get_component<transform_component>(e1)->position.x == 105.0f);
        CHECK(reg.get_component<transform_component>(e2)->position.x == 115.0f);
    }

    SUBCASE("EnTT multi-component sparse-set view") {
        reg.add_component<transform_component>(e1, transform_component{ glm::vec3(1.0f) });
        reg.add_component<mesh_component>(e1, mesh_component{});

        reg.add_component<transform_component>(e2, transform_component{ glm::vec3(2.0f) });
        // e2 does not have mesh_component!

        int view_hits = 0;
        auto view = reg.view<transform_component, mesh_component>();
        for (auto ent : view) {
            ++view_hits;
            CHECK(static_cast<uint32_t>(ent) == e1);
        }
        CHECK(view_hits == 1);
    }

    SUBCASE("Entity deletion cleans up components cleanly") {
        reg.add_component<name_component>(e1, name_component{ "Target" });
        reg.add_component<transform_component>(e1, transform_component{});

        reg.remove_entity(e1);
        CHECK_FALSE(reg.valid(e1));
        CHECK(reg.get_component<name_component>(e1) == nullptr);
        CHECK(reg.get_component<transform_component>(e1) == nullptr);
        CHECK(reg.get_all<name_component>().size() == 0);
    }
}

TEST_CASE("ECS Registry - Entity Duplication and Cloning") {
    registry reg;
    entity e1 = reg.create_entity();
    reg.add_component<name_component>(e1, name_component{ "Enemy" });
    reg.add_component<transform_component>(e1, transform_component{ glm::vec3(3.0f, 4.0f, 5.0f) });

    SUBCASE("Duplicate entity creates valid copy with '_Copy' suffix") {
        entity e_dup = reg.duplicate_entity(e1);
        CHECK(reg.valid(e_dup));
        CHECK(e_dup != e1);
        REQUIRE(reg.has_component<name_component>(e_dup));
        REQUIRE(reg.has_component<transform_component>(e_dup));

        CHECK(reg.get_component<name_component>(e_dup)->name == "Enemy_Copy");
        CHECK(reg.get_component<transform_component>(e_dup)->position.x == 3.0f);
    }

    SUBCASE("Scene clone transfers all entities and components") {
        registry target_reg;
        reg.clone_entity(e1, target_reg);

        CHECK(target_reg.valid(e1));
        REQUIRE(target_reg.has_component<name_component>(e1));
        CHECK(target_reg.get_component<name_component>(e1)->name == "Enemy");
        CHECK(target_reg.get_component<transform_component>(e1)->position.y == 4.0f);
    }
}

TEST_CASE("Primitives - Procedural Cube and Sphere Generation and ARIA Integration") {
    SUBCASE("mesh_generator creates valid cube mesh") {
        auto cube = mesh_generator::generate_cube();
        CHECK(cube.vertices.size() == 24);
        CHECK(cube.indices.size() == 36);
    }

    SUBCASE("mesh_generator creates valid sphere mesh") {
        auto sphere = mesh_generator::generate_sphere(0.5f, 36, 18);
        CHECK(sphere.vertices.size() > 0);
        CHECK(sphere.indices.size() > 0);

        for (const auto& v : sphere.vertices) {
            float dist = glm::length(v.position);
            CHECK(dist == doctest::Approx(0.5f).epsilon(0.01f));
            float normalLen = glm::length(v.normal);
            CHECK(normalLen == doctest::Approx(1.0f).epsilon(0.01f));
        }
    }

    SUBCASE("ARIA loads builtin cube and sphere meshes properly") {
        aria::system ariaSys;
        engine_resources::register_builtin_resources(&ariaSys);

        auto cubeHandle = asset_pipeline::create_cube_mesh(&ariaSys);
        CHECK(cubeHandle.is_valid());
        CHECK(ariaSys.get_mesh_index_count(cubeHandle, 0) == 36);

        auto sphereHandle = asset_pipeline::create_sphere_mesh(&ariaSys);
        CHECK(sphereHandle.is_valid());
        CHECK(ariaSys.get_mesh_index_count(sphereHandle, 0) > 0);

        // Model component ile entegrasyon
        mesh_component comp;
        comp.source = mesh_source_type::primitive_sphere;
        comp.model_handle = sphereHandle;
        CHECK(comp.model_handle.is_valid());
    }
}
