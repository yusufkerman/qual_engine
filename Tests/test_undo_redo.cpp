#include <doctest/doctest.h>
#include "ECS/registry.h"
#include "ECS/components.h"
#include "Editor/Vesta/Adapters/editable_world_impl.h"
#include "Editor/Vesta/CoreState/command_stack.h"
#include "Editor/Vesta/CoreState/Commands/spawn_entity_command.h"
#include "Editor/Vesta/CoreState/Commands/reparent_entity_command.h"
#include "Editor/Vesta/CoreState/Commands/rename_entity_command.h"
#include "Editor/Vesta/CoreState/Commands/compound_command.h"
#include "Editor/Vesta/CoreState/Commands/destroy_entity_command.h"
#include "Editor/Vesta/CoreState/Commands/modify_transform_command.h"

TEST_CASE("Undo/Redo - Command Stack Lifecycle and Boundary Conditions") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    CHECK_FALSE(stack.can_undo());
    CHECK_FALSE(stack.can_redo());

    SUBCASE("Spawn entity command execute and undo/redo") {
        auto spawnCmd = std::make_unique<vesta::spawn_entity_command>("Hero");
        auto* rawCmd = spawnCmd.get();

        stack.execute_command(std::move(spawnCmd));
        CHECK(stack.can_undo());
        CHECK_FALSE(stack.can_redo());
        CHECK(world.is_scene_dirty());

        entity spawned = rawCmd->get_spawned_entity();
        CHECK(spawned != 0);
        CHECK(reg.valid(spawned));
        CHECK(reg.get_component<name_component>(spawned)->name == "Hero");

        // Undo -> Entity should be destroyed
        stack.undo();
        CHECK_FALSE(reg.valid(spawned));
        CHECK_FALSE(stack.can_undo());
        CHECK(stack.can_redo());

        // Redo -> Entity should be recreated
        stack.redo();
        CHECK(stack.can_undo());
        CHECK_FALSE(stack.can_redo());
        CHECK(reg.get_all_entities().size() == 1);
    }
}

TEST_CASE("Undo/Redo - Transform Modification Command") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    entity e = world.spawn_entity("Box");
    auto* t = reg.get_component<transform_component>(e);
    REQUIRE(t != nullptr);
    t->position = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 oldPos(0.0f, 0.0f, 0.0f);
    glm::vec3 newPos(15.0f, 25.0f, -5.0f);
    glm::vec3 oldRot(0.0f);
    glm::vec3 newRot(0.0f, 1.57f, 0.0f);
    glm::vec3 oldScale(1.0f);
    glm::vec3 newScale(2.0f, 2.0f, 2.0f);

    auto transCmd = std::make_unique<vesta::modify_transform_command>(
        e, oldPos, oldRot, oldScale, newPos, newRot, newScale
    );

    stack.execute_command(std::move(transCmd));
    CHECK(t->position.x == doctest::Approx(15.0f));
    CHECK(t->position.y == doctest::Approx(25.0f));
    CHECK(t->position.z == doctest::Approx(-5.0f));
    CHECK(t->scale.x == doctest::Approx(2.0f));

    // Undo -> Restores old position
    stack.undo();
    CHECK(t->position.x == doctest::Approx(0.0f));
    CHECK(t->position.y == doctest::Approx(0.0f));
    CHECK(t->position.z == doctest::Approx(0.0f));
    CHECK(t->scale.x == doctest::Approx(1.0f));

    // Redo -> Reapplies new position
    stack.redo();
    CHECK(t->position.x == doctest::Approx(15.0f));
    CHECK(t->position.y == doctest::Approx(25.0f));
}

TEST_CASE("Undo/Redo - Destroy Entity Command with Component Restoration") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    entity e = world.spawn_entity("TargetEnemy");
    auto* t = reg.get_component<transform_component>(e);
    REQUIRE(t != nullptr);
    t->position = glm::vec3(123.0f, 456.0f, 789.0f);

    auto destroyCmd = std::make_unique<vesta::destroy_entity_command>(e, reg);
    stack.execute_command(std::move(destroyCmd));

    // Entity is removed
    CHECK_FALSE(reg.valid(e));

    // Undo -> Entity and components restored!
    stack.undo();
    CHECK(reg.valid(e));
    REQUIRE(reg.has_component<name_component>(e));
    CHECK(reg.get_component<name_component>(e)->name == "TargetEnemy");
    REQUIRE(reg.has_component<transform_component>(e));
    CHECK(reg.get_component<transform_component>(e)->position.x == doctest::Approx(123.0f));
    CHECK(reg.get_component<transform_component>(e)->position.y == doctest::Approx(456.0f));
    CHECK(reg.get_component<transform_component>(e)->position.z == doctest::Approx(789.0f));
}

TEST_CASE("Hierarchy - Recursive Child Destruction on Parent Destroy") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);

    entity parent = world.spawn_entity("Parent");
    entity child1 = world.spawn_entity("Child1");
    entity child2 = world.spawn_entity("Child2");
    entity grandChild = world.spawn_entity("GrandChild");

    world.set_entity_parent(child1, parent);
    world.set_entity_parent(child2, parent);
    world.set_entity_parent(grandChild, child1);

    CHECK(reg.valid(parent));
    CHECK(reg.valid(child1));
    CHECK(reg.valid(child2));
    CHECK(reg.valid(grandChild));

    // Destroy parent -> all children and descendants must be destroyed recursively
    world.destroy_entity(parent);

    CHECK_FALSE(reg.valid(parent));
    CHECK_FALSE(reg.valid(child1));
    CHECK_FALSE(reg.valid(child2));
    CHECK_FALSE(reg.valid(grandChild));
}

TEST_CASE("Undo/Redo - Destroy Parent with Recursive Descendants Restoration") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    entity parent = world.spawn_entity("Parent");
    entity child = world.spawn_entity("Child");
    entity grandChild = world.spawn_entity("GrandChild");

    world.set_entity_parent(child, parent);
    world.set_entity_parent(grandChild, child);

    auto destroyCmd = std::make_unique<vesta::destroy_entity_command>(parent, reg);
    stack.execute_command(std::move(destroyCmd));

    // Everything destroyed
    CHECK_FALSE(reg.valid(parent));
    CHECK_FALSE(reg.valid(child));
    CHECK_FALSE(reg.valid(grandChild));

    // Undo -> Parent, Child, and GrandChild restored
    stack.undo();
    CHECK(reg.valid(parent));
    CHECK(reg.valid(child));
    CHECK(reg.valid(grandChild));
    CHECK(reg.get_component<name_component>(parent)->name == "Parent");
    CHECK(reg.get_component<name_component>(child)->name == "Child");
    CHECK(reg.get_component<name_component>(grandChild)->name == "GrandChild");

    // Redo -> Destroyed again
    stack.redo();
    CHECK_FALSE(reg.valid(parent));
    CHECK_FALSE(reg.valid(child));
    CHECK_FALSE(reg.valid(grandChild));
}

TEST_CASE("Undo/Redo - Reparent Entity Command Batch and Single") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    entity parentA = world.spawn_entity("ParentA");
    entity parentB = world.spawn_entity("ParentB");
    entity child1 = world.spawn_entity("Child1");
    entity child2 = world.spawn_entity("Child2");

    world.set_entity_parent(child1, parentA);
    world.set_entity_parent(child2, parentA);

    CHECK(reg.get_component<hierarchy_component>(child1)->parent == parentA);
    CHECK(reg.get_component<hierarchy_component>(child2)->parent == parentA);

    // Batch reparent child1 and child2 to parentB
    auto reparentCmd = std::make_unique<vesta::reparent_entity_command>(
        std::vector<entity>{ child1, child2 }, parentB, reg
    );
    stack.execute_command(std::move(reparentCmd));

    CHECK(reg.get_component<hierarchy_component>(child1)->parent == parentB);
    CHECK(reg.get_component<hierarchy_component>(child2)->parent == parentB);

    // Undo -> restored to parentA
    stack.undo();
    CHECK(reg.get_component<hierarchy_component>(child1)->parent == parentA);
    CHECK(reg.get_component<hierarchy_component>(child2)->parent == parentA);

    // Redo -> reparented to parentB
    stack.redo();
    CHECK(reg.get_component<hierarchy_component>(child1)->parent == parentB);
    CHECK(reg.get_component<hierarchy_component>(child2)->parent == parentB);
}

TEST_CASE("Undo/Redo - Rename Entity Command") {
    registry reg;
    vesta::editable_world_impl world(reg, nullptr);
    vesta::command_stack stack(world);

    entity e = world.spawn_entity("OriginalName");
    CHECK(reg.get_component<name_component>(e)->name == "OriginalName");

    auto renameCmd = std::make_unique<vesta::rename_entity_command>(e, "OriginalName", "RenamedEntity");
    stack.execute_command(std::move(renameCmd));
    CHECK(reg.get_component<name_component>(e)->name == "RenamedEntity");

    stack.undo();
    CHECK(reg.get_component<name_component>(e)->name == "OriginalName");

    stack.redo();
    CHECK(reg.get_component<name_component>(e)->name == "RenamedEntity");
}
