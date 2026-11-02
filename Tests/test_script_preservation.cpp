#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include "ECS/registry.h"
#include "ECS/components.h"
#include "Scene/scriptable_entity.h"
#include "Engine/Scripting/script_factory.h"
#include "Scene/scene_serializer.h"
#include "Scene/scene.h"
#include "Engine/engine.h"

namespace
{
    class TestPlayerScript : public scriptable_entity
    {
    public:
        int health = 100;
        float mana = 50.0f;
        std::string player_name = "DefaultHero";

        std::string serialize_state() const override
        {
            nlohmann::json j;
            j["health"] = health;
            j["mana"] = mana;
            j["player_name"] = player_name;
            return j.dump();
        }

        void deserialize_state(const std::string& state) override
        {
            auto j = nlohmann::json::parse(state, nullptr, false);
            if (!j.is_discarded())
            {
                health = j.value("health", 100);
                mana = j.value("mana", 50.0f);
                player_name = j.value("player_name", "DefaultHero");
            }
        }
    };
}

TEST_SUITE("ScriptStatePreservation")
{
    TEST_CASE("ScriptState: Virtual serialize and deserialize methods")
    {
        TestPlayerScript script1;
        script1.health = 42;
        script1.mana = 18.5f;
        script1.player_name = "QualKnight";

        const std::string serialized = script1.serialize_state();
        CHECK(!serialized.empty());

        TestPlayerScript script2;
        CHECK(script2.health == 100);
        CHECK(script2.player_name == "DefaultHero");

        script2.deserialize_state(serialized);
        CHECK(script2.health == 42);
        CHECK(script2.mana == doctest::Approx(18.5f));
        CHECK(script2.player_name == "QualKnight");
    }

    TEST_CASE("ScriptState: Simulated DLL Hot-Reload state retention")
    {
        registry reg;
        const entity e = reg.create_entity();

        auto& nsc = reg.add_component<native_script_component>(e);
        nsc.script_name = "TestPlayerScript";
        auto* originalInstance = new TestPlayerScript();
        originalInstance->health = 15;
        originalInstance->mana = 88.0f;
        originalInstance->player_name = "VestaCommander";
        nsc.instance = originalInstance;

        // Step 1: Simulate Unload (DLL FreeLibrary before hot-reload)
        CHECK(nsc.saved_state.empty());
        if (nsc.instance)
        {
            nsc.saved_state = nsc.instance->serialize_state();
            delete nsc.instance;
            nsc.instance = nullptr;
        }

        CHECK(nsc.instance == nullptr);
        CHECK(!nsc.saved_state.empty());

        // Step 2: Simulate Reload (DLL LoadLibrary after recompile)
        auto* newInstance = new TestPlayerScript();
        nsc.instance = newInstance;

        // Initial default state
        CHECK(newInstance->health == 100);

        // Restore state
        if (!nsc.saved_state.empty())
        {
            nsc.instance->deserialize_state(nsc.saved_state);
            nsc.saved_state.clear();
        }

        CHECK(newInstance->health == 15);
        CHECK(newInstance->mana == doctest::Approx(88.0f));
        CHECK(newInstance->player_name == "VestaCommander");
        CHECK(nsc.saved_state.empty());

        delete newInstance;
        nsc.instance = nullptr;
    }

    TEST_CASE("ScriptState: Scene Serialization integration")
    {
        script_factory::register_script("TestPlayerScript", [](entity ent, registry& r) {
            auto* pComp = r.get_component<native_script_component>(ent);
            auto& comp = pComp ? *pComp : r.add_component<native_script_component>(ent);
            comp.bind<TestPlayerScript>("TestPlayerScript");
            comp.instance = comp.bind_instantiate ? comp.bind_instantiate() : nullptr;
        });

        scene srcScene;
        EngineContext ctx{};
        const entity e = srcScene.create_entity();
        name_component nameComp;
        nameComp.name = "HeroEntity";
        srcScene.get_registry().add_component<name_component>(e, nameComp);

        auto& nsc = srcScene.get_registry().add_component<native_script_component>(e);
        nsc.script_name = "TestPlayerScript";
        auto* pScript = new TestPlayerScript();
        pScript->health = 77;
        pScript->mana = 99.5f;
        pScript->player_name = "ArchMage";
        nsc.instance = pScript;

        const std::string tempScenePath = (std::filesystem::temp_directory_path() / "qual_test_scene_state.json").string();

        // Serialize scene
        const bool saved = scene_serializer::serialize(tempScenePath, srcScene, ctx);
        CHECK(saved);

        // Verify JSON contents have saved_state
        std::ifstream f(tempScenePath);
        nlohmann::json j;
        f >> j;
        f.close();

        CHECK(j.contains("entities"));
        CHECK(j["entities"].size() == 1);
        CHECK(j["entities"][0].contains("native_script_component"));
        CHECK(j["entities"][0]["native_script_component"].contains("saved_state"));

        // Deserialize scene into new scene
        scene dstScene;
        const bool loaded = scene_serializer::deserialize(tempScenePath, dstScene, ctx);
        CHECK(loaded);

        entity loadedEntity = 0;
        for (const auto& [dstE, dstName] : dstScene.get_registry().get_all<name_component>())
        {
            if (dstName.name == "HeroEntity")
            {
                loadedEntity = dstE;
                break;
            }
        }
        CHECK(loadedEntity != 0);

        auto* loadedNsc = dstScene.get_registry().get_component<native_script_component>(loadedEntity);
        REQUIRE(loadedNsc != nullptr);
        REQUIRE(loadedNsc->instance != nullptr);

        auto* loadedScript = dynamic_cast<TestPlayerScript*>(loadedNsc->instance);
        REQUIRE(loadedScript != nullptr);
        CHECK(loadedScript->health == 77);
        CHECK(loadedScript->mana == doctest::Approx(99.5f));
        CHECK(loadedScript->player_name == "ArchMage");

        delete pScript;
        nsc.instance = nullptr;

        std::filesystem::remove(tempScenePath);
        script_factory::unregister_all_scripts();
    }
}
