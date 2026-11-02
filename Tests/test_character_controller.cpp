#include <doctest/doctest.h>
#include "../Engine/ECS/registry.h"
#include "../Engine/ECS/components.h"
#include "../Engine/Gameplay/character_controller_wrapper.h"
#include "../Engine/Gameplay/gameplay_script.h"
#include "../Engine/Systems/physics_system.h"
#include "../Engine/Systems/physics_bridge.h"
#include "../Engine/Engine/engine_context.h"
#include "../Engine/Scene/scene.h"

TEST_SUITE("KinematicCharacterController")
{
    TEST_CASE("CharacterController: Default values and ECS lifecycle")
    {
        registry reg;
        const entity e = reg.create_entity();

        auto& cc = reg.add_component<character_controller_component>(e);
        CHECK(cc.radius == doctest::Approx(0.4f));
        CHECK(cc.height == doctest::Approx(1.8f));
        CHECK(cc.walk_speed == doctest::Approx(5.0f));
        CHECK(cc.jump_speed == doctest::Approx(5.0f));
        CHECK(cc.max_slope_angle == doctest::Approx(45.0f));
        CHECK(cc.step_height == doctest::Approx(0.35f));
        CHECK(cc.gravity == doctest::Approx(-9.81f));
        CHECK(cc.is_grounded == false);
        CHECK(cc.character == nullptr);

        // Verification of wrapper abstraction
        gameplay::character_controller_wrapper wrapper(e, &reg);
        CHECK(wrapper.is_valid());
        CHECK(wrapper.is_grounded() == false);
        CHECK(wrapper.get_walk_speed() == doctest::Approx(5.0f));

        wrapper.set_walk_speed(7.5f);
        CHECK(wrapper.get_walk_speed() == doctest::Approx(7.5f));
        CHECK(cc.walk_speed == doctest::Approx(7.5f));

        wrapper.set_step_height(0.5f);
        CHECK(wrapper.get_step_height() == doctest::Approx(0.5f));
        CHECK(cc.step_height == doctest::Approx(0.5f));

        wrapper.move(glm::vec3(3.0f, 0.0f, -4.0f));
        CHECK(cc.linear_velocity.x == doctest::Approx(3.0f));
        CHECK(cc.linear_velocity.z == doctest::Approx(-4.0f));

        // Jump when not grounded does not activate
        wrapper.jump();
        CHECK(cc.linear_velocity.y == doctest::Approx(0.0f));

        // When grounded, jump applies upward velocity
        cc.is_grounded = true;
        wrapper.jump(6.5f);
        CHECK(cc.linear_velocity.y == doctest::Approx(6.5f));
        CHECK(cc.is_grounded == false);
    }

    TEST_CASE("CharacterController: Jolt Physical Simulation, Gravity & Grounding")
    {
        EngineContext ctx;
        scene test_scene;

        // Initialize Jolt physics
        const bool phys_ok = ctx.physicsSystem.init(test_scene);
        CHECK(phys_ok);

        auto& reg = test_scene.get_registry();

        // 1. Create static ground floor
        const entity floor = reg.create_entity();
        auto& floor_t = reg.add_component<transform_component>(floor);
        floor_t.position = glm::vec3(0.0f, 0.0f, 0.0f);

        auto& floor_col = reg.add_component<collider_component>(floor);
        floor_col.type = collider_type::box;
        floor_col.extents = glm::vec3(50.0f, 0.5f, 50.0f); // half extents (height = 1.0)

        auto& floor_rb = reg.add_component<rigidbody_component>(floor);
        floor_rb.motion = motion_type::static_body;

        // 2. Create Character Controller above ground (y = 4.0f)
        const entity player = reg.create_entity();
        auto& player_t = reg.add_component<transform_component>(player);
        player_t.position = glm::vec3(0.0f, 4.0f, 0.0f);

        auto& player_cc = reg.add_component<character_controller_component>(player);
        player_cc.radius = 0.4f;
        player_cc.height = 1.8f;
        player_cc.gravity = -9.81f;
        player_cc.jump_speed = 6.0f;
        player_cc.walk_speed = 4.0f;

        // Build Jolt physics world
        physics_bridge::create_physics_world(ctx, test_scene);
        CHECK(player_cc.character != nullptr);

        gameplay::character_controller_wrapper ctrl(player, &reg, &ctx.physicsSystem);
        CHECK(ctrl.is_valid());
        CHECK(ctrl.is_grounded() == false);

        // Step simulation 60 frames (1 second of physics at 60 Hz)
        constexpr float dt = 1.0f / 60.0f;
        for (int step = 0; step < 60; ++step)
        {
            physics_bridge::update_character_controllers(ctx, test_scene, dt);
            ctx.physicsSystem.update(dt);
        }

        // After falling, player should be on ground!
        CHECK(ctrl.is_grounded() == true);
        CHECK(player_t.position.y < 3.5f);
        CHECK(player_t.position.y >= 0.0f); // above floor surface

        // Now perform a jump while grounded
        ctrl.jump();
        CHECK(ctrl.is_grounded() == false);
        CHECK(ctrl.get_velocity().y > 0.0f);

        // Advance 5 steps: player should be moving upwards
        const float pre_jump_y = player_t.position.y;
        for (int step = 0; step < 5; ++step)
        {
            physics_bridge::update_character_controllers(ctx, test_scene, dt);
            ctx.physicsSystem.update(dt);
        }
        CHECK(player_t.position.y > pre_jump_y);

        // Move horizontally
        ctrl.move(glm::vec3(4.0f, 0.0f, 0.0f));
        const float pre_move_x = player_t.position.x;
        for (int step = 0; step < 10; ++step)
        {
            physics_bridge::update_character_controllers(ctx, test_scene, dt);
            ctx.physicsSystem.update(dt);
        }
        CHECK(player_t.position.x > pre_move_x);

        // Cleanup
        physics_bridge::destroy_physics_world(ctx, test_scene);
        CHECK(player_cc.character == nullptr);

        ctx.physicsSystem.shutdown();
    }
}
