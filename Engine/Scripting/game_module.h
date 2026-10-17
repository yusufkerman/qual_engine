#pragma once

/**
 * @file game_module.h
 * @brief Projeye özel C++ betiklerinin (Scripts) bağımsız bir dinamik kütüphane (GameModule.dll)
 * olarak derlenmesini ve motora bağlanmasını sağlayan modül arayüzü ve makroları.
 */

#include <string>
#include <vector>
#include <functional>
#include "../Gameplay/gameplay_context.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>

class scriptable_entity;

#ifdef _WIN32
    #define GAME_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
    #define GAME_MODULE_EXPORT extern "C"
#endif

// Modül C-linkage fonksiyon imzaları
typedef int (*GetScriptCountFn)();
typedef const char* (*GetScriptNameFn)(int index);
typedef scriptable_entity* (*InstantiateScriptFn)(const char* name);
typedef void (*DestroyScriptFn)(scriptable_entity* instance);
typedef void (*SyncModuleContextFn)(EngineContext* engineCtx, scene* activeScene, gameplay::collision_event_queue* collisionQueue);

/**
 * @brief Kullanıcı projelerinde GameModule.cpp içinde kullanılacak kayıt makroları.
 * Örnek kullanım:
 * 
 * GAME_MODULE_BEGIN()
 *     GAME_MODULE_SCRIPT(player_controller)
 *     GAME_MODULE_SCRIPT(camera_controller)
 * GAME_MODULE_END()
 */
#define GAME_MODULE_BEGIN() \
    namespace { \
        struct ModuleScriptEntry { \
            const char* name; \
            scriptable_entity* (*create)(); \
            void (*destroy)(scriptable_entity*); \
        }; \
        static const ModuleScriptEntry s_module_scripts[] = {

#define GAME_MODULE_SCRIPT(ScriptClass) \
            { \
                #ScriptClass, \
                []() -> scriptable_entity* { return static_cast<scriptable_entity*>(new ScriptClass()); }, \
                [](scriptable_entity* inst) { delete static_cast<ScriptClass*>(inst); } \
            },

#define GAME_MODULE_END() \
            { nullptr, nullptr, nullptr } \
        }; \
        static const int s_module_script_count = static_cast<int>((sizeof(s_module_scripts) / sizeof(s_module_scripts[0])) - 1); \
    } \
    GAME_MODULE_EXPORT int GetScriptCount() { \
        return s_module_script_count; \
    } \
    GAME_MODULE_EXPORT const char* GetScriptName(int index) { \
        if (index >= 0 && index < s_module_script_count) { \
            return s_module_scripts[index].name; \
        } \
        return nullptr; \
    } \
    GAME_MODULE_EXPORT scriptable_entity* InstantiateScript(const char* name) { \
        if (!name) return nullptr; \
        for (int i = 0; i < s_module_script_count; ++i) { \
            if (s_module_scripts[i].name && std::string(s_module_scripts[i].name) == name) { \
                return s_module_scripts[i].create(); \
            } \
        } \
        return nullptr; \
    } \
    GAME_MODULE_EXPORT void DestroyScript(scriptable_entity* instance) { \
        if (!instance) return; \
        delete instance; \
    } \
    GAME_MODULE_EXPORT void SyncModuleContext(EngineContext* engineCtx, scene* activeScene, gameplay::collision_event_queue* collisionQueue) { \
        static bool s_joltAllocRegistered = false; \
        if (!s_joltAllocRegistered) { \
            JPH::RegisterDefaultAllocator(); \
            s_joltAllocRegistered = true; \
        } \
        gameplay::context::set_engine_context(engineCtx); \
        gameplay::context::set_active_scene(activeScene); \
        gameplay::context::set_collision_queue(collisionQueue); \
    }


