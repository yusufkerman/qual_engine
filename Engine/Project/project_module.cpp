/**
 * @file project_module.cpp
 * @brief Dinamik oyun modülü yükleyicisi uygulaması.
 */

#include "project_module.h"
#include "project.h"
#include "../Scripting/game_module.h"
#include "../Engine/Scripting/script_factory.h"
#include "../Scene/scene.h"
#include "../ECS/components.h"
#include "../Engine/engine.h"
#include "../Resources/engine_resources.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <set>

namespace project {

    namespace {
        std::filesystem::path resolve_engine_repo_root() {
            try {
                const auto& res = engine_resources::get_resources_path();
                if (!res.empty()) {
                    auto engineDir = res.parent_path();
                    auto repoRoot = engineDir.parent_path();
                    if (std::filesystem::exists(repoRoot / "Engine" / "Engine.vcxproj")) {
                        return std::filesystem::canonical(repoRoot);
                    }
                }
            } catch (...) {}

            try {
                char buffer[MAX_PATH];
                DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
                if (len > 0) {
                    auto exeDir = std::filesystem::path(buffer).parent_path();
                    auto cand1 = exeDir.parent_path();
                    if (std::filesystem::exists(cand1 / "Engine" / "Engine.vcxproj")) {
                        return std::filesystem::canonical(cand1);
                    }
                    auto cand2 = cand1.parent_path();
                    if (std::filesystem::exists(cand2 / "Engine" / "Engine.vcxproj")) {
                        return std::filesystem::canonical(cand2);
                    }
                }
            } catch (...) {}

            try {
                auto cwd = std::filesystem::current_path();
                if (std::filesystem::exists(cwd / "Engine" / "Engine.vcxproj")) {
                    return std::filesystem::canonical(cwd);
                }
            } catch (...) {}

            return "c:/Users/yusuf/source/repos/OpenGL-Learn-1";
        }
    } // anonymous namespace

    project_module& project_module::instance() {
        static project_module s_instance;
        return s_instance;
    }

    project_module::~project_module() {
        unload_module();
    }

    bool project_module::load_module(const std::string& dllPath) {
        unload_module();

        if (dllPath.empty() || !std::filesystem::exists(dllPath)) {
            std::cerr << "[ProjectModule] Hata: Modul dosyasi bulunamadi: " << dllPath << "\n";
            return false;
        }

        try {
            const std::filesystem::path origPath(dllPath);
            const std::filesystem::path dir = origPath.parent_path();
            const std::filesystem::path runtimePath = dir / "GameModule_runtime.dll";

            // 1. Orijinal DLL dosyasının Windows tarafından kilitlenmesini önlemek için çalışma kopyası üret
            std::error_code ec;
            std::filesystem::copy_file(origPath, runtimePath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                std::cerr << "[ProjectModule] Uyari: Calisma kopyasi olusturulamadi, dogrudan yuklenecek: " << ec.message() << "\n";
            }

            const std::string loadPath = std::filesystem::exists(runtimePath) ? runtimePath.string() : origPath.string();

            // PDB sembol dosyasını da kopyala (Debug sırasındaki semboller için)
            const std::filesystem::path origPdb = dir / (origPath.stem().string() + ".pdb");
            const std::filesystem::path runtimePdb = dir / "GameModule_runtime.pdb";
            if (std::filesystem::exists(origPdb)) {
                std::filesystem::copy_file(origPdb, runtimePdb, std::filesystem::copy_options::overwrite_existing, ec);
            }

            // 2. DLL'i belleğe yükle
            HMODULE hMod = LoadLibraryA(loadPath.c_str());
            if (!hMod) {
                DWORD err = GetLastError();
                std::cerr << "[ProjectModule] Hata: DLL yuklenemedi (Kod: " << err << "): " << loadPath << "\n";
                return false;
            }

            // 3. Fonksiyon göstericilerini çözümle
            auto pGetCount = reinterpret_cast<GetScriptCountFn>(GetProcAddress(hMod, "GetScriptCount"));
            auto pGetName = reinterpret_cast<GetScriptNameFn>(GetProcAddress(hMod, "GetScriptName"));
            auto pInstantiate = reinterpret_cast<InstantiateScriptFn>(GetProcAddress(hMod, "InstantiateScript"));
            auto pDestroy = reinterpret_cast<DestroyScriptFn>(GetProcAddress(hMod, "DestroyScript"));
            auto pSyncContext = reinterpret_cast<SyncModuleContextFn>(GetProcAddress(hMod, "SyncModuleContext"));

            if (!pGetCount || !pGetName || !pInstantiate || !pDestroy) {
                std::cerr << "[ProjectModule] Hata: Modulde zorunlu C-linkage fonksiyonlar bulunamadi!\n";
                FreeLibrary(hMod);
                return false;
            }

            m_hModule = hMod;
            m_syncContextFn = reinterpret_cast<void*>(pSyncContext);
            m_isLoaded = true;
            m_originalDllPath = origPath.string();
            m_runtimeDllPath = loadPath;
            m_loadedScripts.clear();

            // DLL içerisindeki statik motor bağlamını ve Jolt bellek ayırıcısını hemen senkronize et
            sync_context();

            // 4. Modüldeki tüm scriptleri script_factory'ye bağla
            int count = pGetCount();
            for (int i = 0; i < count; ++i) {
                const char* nameStr = pGetName(i);
                if (nameStr) {
                    std::string scriptName = nameStr;
                    m_loadedScripts.push_back(scriptName);

                    script_factory::register_script(scriptName, [pInstantiate, pDestroy, scriptName](entity e, registry& reg) {
                        auto& nsc = reg.add_component<native_script_component>(e);
                        nsc.script_name = scriptName;
                        nsc.bind_instantiate = [pInstantiate, scriptName]() -> scriptable_entity* {
                            project_module::instance().sync_context();
                            return pInstantiate(scriptName.c_str());
                        };
                        nsc.bind_destroy = [pDestroy](native_script_component* comp) {
                            if (comp && comp->instance) {
                                pDestroy(comp->instance);
                                comp->instance = nullptr;
                            }
                        };
                    });
                }
            }

            // 5. Sahnedeki mevcut tüm native_script_component'leri yeni yüklenen modüle hemen bağla
            scene* activeScene = gameplay::context::get_active_scene();
            if (activeScene) {
                registry& reg = activeScene->get_registry();
                std::vector<std::pair<entity, std::string>> scriptsToBind;
                for (const auto& [e, scriptComp] : reg.get_all<native_script_component>()) {
                    if (!scriptComp.script_name.empty() && scriptComp.script_name != "None") {
                        scriptsToBind.emplace_back(e, scriptComp.script_name);
                    }
                }
                for (const auto& [e, name] : scriptsToBind) {
                    script_factory::instantiate_script(name, e, reg);
                }
            }

            std::cout << "[ProjectModule] Basarili: GameModule yuklendi (" << count << " script kaydedildi):\n";
            for (const auto& s : m_loadedScripts) {
                std::cout << "  - [Script] " << s << "\n";
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectModule] Modul yukleme sirasinda istisna: " << e.what() << "\n";
            return false;
        }
    }

    void project_module::sync_context() const {
        if (!m_isLoaded || !m_syncContextFn) return;
        auto fn = reinterpret_cast<SyncModuleContextFn>(m_syncContextFn);
        auto* eCtx = gameplay::context::get_engine_context();
        physics_system* pSys = eCtx ? &eCtx->physicsSystem : nullptr;
        InputState* iState = eCtx ? &eCtx->inputState : nullptr;
        fn(eCtx,
           gameplay::context::get_active_scene(),
           gameplay::context::get_collision_queue(),
           pSys,
           iState);
    }

    void project_module::unload_module() {
        if (!m_isLoaded && !m_hModule) return;

        // Sahnedeki aktif betik örneklerini temizle ve göstericileri sıfırla
        scene* activeScene = gameplay::context::get_active_scene();
        if (activeScene) {
            activeScene->clean_up_scripts();
            registry& reg = activeScene->get_registry();
            for (auto& [e, scriptComp] : reg.get_all<native_script_component>()) {
                scriptComp.instance = nullptr;
                scriptComp.bind_instantiate = nullptr;
                scriptComp.bind_destroy = nullptr;
            }
        }

        // Fabrikadaki script kayıtlarını temizle
        script_factory::unregister_all_scripts();
        m_loadedScripts.clear();
        m_syncContextFn = nullptr;

        if (m_hModule) {
            FreeLibrary(static_cast<HMODULE>(m_hModule));
            m_hModule = nullptr;
        }

        m_isLoaded = false;

        // Geçici çalışma kopyasını temizle
        if (!m_runtimeDllPath.empty() && m_runtimeDllPath != m_originalDllPath) {
            std::error_code ec;
            std::filesystem::remove(m_runtimeDllPath, ec);
            std::filesystem::remove(std::filesystem::path(m_runtimeDllPath).parent_path() / "GameModule_runtime.pdb", ec);
        }

        std::cout << "[ProjectModule] Modul bellekten bosaltildi ve script kayitlari temizlendi.\n";
    }

    bool project_module::reload_module() {
        if (m_originalDllPath.empty()) {
            const auto& root = project_manager::instance().get_project_root();
            if (!root.empty()) {
                const auto defaultDll = std::filesystem::path(root) / "Binaries" / "GameModule.dll";
                if (std::filesystem::exists(defaultDll)) {
                    return load_module(defaultDll.string());
                }
            }
            return false;
        }
        std::string pathCopy = m_originalDllPath;
        return load_module(pathCopy);
    }

    bool project_module::is_module_outdated(const std::string& projectRoot) {
        namespace fs = std::filesystem;
        if (projectRoot.empty()) return false;
        try {
            const fs::path root(projectRoot);
            const fs::path targetDll = root / "Binaries" / "GameModule.dll";
            if (!fs::exists(targetDll)) return true;

            const auto dllTime = fs::last_write_time(targetDll);

            // 1. Engine.lib / repo kütüphanesi kontrolü
            const auto repoRoot = resolve_engine_repo_root();
            if (!repoRoot.empty()) {
                const fs::path engineLib = repoRoot / "x64" / "Debug" / "Engine.lib";
                if (fs::exists(engineLib) && fs::last_write_time(engineLib) > dllTime) {
                    return true;
                }
            }

            // 2. Proje C++ betikleri kontrolü
            const std::vector<fs::path> dirs = {
                root / "Assets" / "Scripts",
                root / "Scripts"
            };
            for (const auto& dir : dirs) {
                if (!fs::exists(dir)) continue;
                for (const auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
                        if (entry.path().filename() == "GameModule.cpp") continue;
                        if (entry.last_write_time() > dllTime) {
                            return true;
                        }
                    }
                }
            }
        } catch (...) {}
        return false;
    }

    bool project_module::compile_project_scripts(const std::string& projectRoot, std::string& outLog) {
        if (projectRoot.empty()) {
            outLog = "Error: Project path is not specified.";
            return false;
        }

        // Derlemeden önce yüklü olan DLL'i bellekten boşalt ki linker dosya kilidiyle karşılaşmasın
        instance().unload_module();

        // Kilitli olmayan eski PDB dosyalarını temizle
        try {
            const std::filesystem::path binDir = std::filesystem::path(projectRoot) / "Binaries";
            if (std::filesystem::exists(binDir)) {
                for (const auto& entry : std::filesystem::directory_iterator(binDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".pdb") {
                        std::error_code ec;
                        std::filesystem::remove(entry.path(), ec);
                    }
                }
            }
        } catch (...) {}

        // 1. Proje iskeletini ve betik kayitlarini tara, eksikse olustur
        std::string scaffoldLog;
        if (!ensure_script_scaffolding(projectRoot, scaffoldLog)) {
            outLog = "Scaffolding error: " + scaffoldLog;
            return false;
        }

        const std::filesystem::path root(projectRoot);
        const std::filesystem::path batScript = root / "Scripts" / "build_module.bat";
        const std::filesystem::path targetDll = root / "Binaries" / "GameModule.dll";

        // 2. Hazirlanan build_module.bat uzerinden hizli derleme (devenv / MSBuild) calistir
        if (std::filesystem::exists(batScript)) {
            std::string cmd = "cmd.exe /c \"" + batScript.string() + "\" 2>&1";
            FILE* pipe = _popen(cmd.c_str(), "r");
            if (!pipe) {
                outLog = "Error: Compilation process could not be started.";
                return false;
            }

            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                outLog += buffer;
            }
            int code = _pclose(pipe);
            bool success = (code == 0) && std::filesystem::exists(targetDll);
            if (success) {
                instance().load_module(targetDll.string());
            }
            return success;
        }

        outLog = "Error: build_module.bat was not found: " + batScript.string();
        return false;
    }

    bool project_module::ensure_script_scaffolding(const std::string& projectRoot, std::string& outLog) {
        if (projectRoot.empty()) {
            outLog = "Gecersiz proje ana dizini.";
            return false;
        }

        try {
            const std::filesystem::path root(projectRoot);
            const std::filesystem::path assetsDir = root / "Assets";
            const std::filesystem::path scriptsAssetDir = assetsDir / "Scripts";
            const std::filesystem::path binariesDir = root / "Binaries";
            const std::filesystem::path projScriptsDir = root / "Scripts";

            std::filesystem::create_directories(scriptsAssetDir);
            std::filesystem::create_directories(binariesDir);
            std::filesystem::create_directories(projScriptsDir);

            const auto repoRoot = resolve_engine_repo_root();
            const auto engineDir = repoRoot / "Engine";
            const auto debugLibDir = repoRoot / "x64" / "Debug";
            const auto releaseLibDir = repoRoot / "x64" / "Release";

            // 1. Assets ve Scripts altindaki tum .h / .hpp dosyalarini ve script siniflarini tara
            struct DiscoveredScript {
                std::string className;
                std::string headerName;
            };
            std::vector<DiscoveredScript> discoveredScripts;
            std::set<std::string> uniqueClassNames;
            std::set<std::string> uniqueHeaderIncludes;

            std::vector<std::string> clCompileFiles;
            std::vector<std::string> clIncludeFiles;

            const std::regex scriptRegex(R"(class\s+(?:[A-Za-z0-9_]+\s+)?([A-Za-z0-9_]+)\s*:\s*[^\{]*(?:gameplay::script|scriptable_entity|script)\b)");

            auto scanDirectory = [&](const std::filesystem::path& dir) {
                if (!std::filesystem::exists(dir)) return;
                for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                    if (!entry.is_regular_file()) continue;
                    const auto ext = entry.path().extension().string();
                    const std::string filename = entry.path().filename().string();

                    const std::string fullPathStr = entry.path().string();
                    if (fullPathStr.find("\\obj\\") != std::string::npos || fullPathStr.find("/obj/") != std::string::npos) {
                        continue;
                    }

                    if (ext == ".cpp") {
                        if (filename != "GameModule.cpp") {
                            std::error_code ec;
                            auto rel = std::filesystem::relative(entry.path(), projScriptsDir, ec);
                            if (!ec) {
                                clCompileFiles.push_back(rel.string());
                            }
                        }
                    } else if (ext == ".h" || ext == ".hpp") {
                        std::error_code ec;
                        auto rel = std::filesystem::relative(entry.path(), projScriptsDir, ec);
                        if (!ec) {
                            clIncludeFiles.push_back(rel.string());
                        }

                        // Dosya iceriginde script sinifi ara
                        std::ifstream inFile(entry.path());
                        if (inFile.is_open()) {
                            std::stringstream ss;
                            ss << inFile.rdbuf();
                            std::string content = ss.str();
                            inFile.close();

                            auto words_begin = std::sregex_iterator(content.begin(), content.end(), scriptRegex);
                            auto words_end = std::sregex_iterator();
                            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                                std::smatch match = *i;
                                std::string cls = match[1].str();
                                if (uniqueClassNames.find(cls) == uniqueClassNames.end()) {
                                    uniqueClassNames.insert(cls);
                                    uniqueHeaderIncludes.insert(filename);
                                    discoveredScripts.push_back({ cls, filename });
                                }
                            }
                        }
                    }
                }
            };

            scanDirectory(assetsDir);
            scanDirectory(projScriptsDir);

            // 2. GameModule.cpp olustur / guncelle
            const std::filesystem::path gameModuleCppPath = scriptsAssetDir / "GameModule.cpp";
            bool shouldWriteGameModule = true;

            if (std::filesystem::exists(gameModuleCppPath)) {
                std::ifstream currentGm(gameModuleCppPath);
                if (currentGm.is_open()) {
                    std::string firstLine;
                    std::getline(currentGm, firstLine);
                    currentGm.close();
                    if (firstLine.find("[AUTO-GENERATED-BY-ENGINE]") == std::string::npos) {
                        shouldWriteGameModule = false;
                    }
                }
            }

            if (shouldWriteGameModule) {
                std::stringstream gm;
                gm << "// [AUTO-GENERATED-BY-ENGINE]\n";
                gm << "/**\n";
                gm << " * @file GameModule.cpp\n";
                gm << " * @brief Dinamik oyun modulu kayit noktasi.\n";
                gm << " * Bu dosya motor tarafindan otomatik olusturulmustur.\n";
                gm << " */\n\n";
                gm << "#include <Engine/Scripting/game_module.h>\n";
                for (const auto& h : uniqueHeaderIncludes) {
                    gm << "#include \"" << h << "\"\n";
                }
                gm << "\nGAME_MODULE_BEGIN()\n";
                for (const auto& s : discoveredScripts) {
                    gm << "    GAME_MODULE_SCRIPT(" << s.className << ")\n";
                }
                gm << "GAME_MODULE_END()\n";

                std::string newGmContent = gm.str();
                std::string existingGmContent;
                if (std::filesystem::exists(gameModuleCppPath)) {
                    std::ifstream inGm(gameModuleCppPath);
                    std::stringstream ssgm;
                    ssgm << inGm.rdbuf();
                    existingGmContent = ssgm.str();
                }

                if (existingGmContent != newGmContent) {
                    std::ofstream outGm(gameModuleCppPath);
                    outGm << newGmContent;
                    outGm.close();
                }
            }

            // GameModule.cpp dosyasini ClCompile listesine ekle
            std::error_code ec;
            auto gmRel = std::filesystem::relative(gameModuleCppPath, projScriptsDir, ec);
            if (!ec) {
                clCompileFiles.push_back(gmRel.string());
            }

            // 3. GameModule.vcxproj olustur / guncelle
            const std::filesystem::path vcxprojPath = projScriptsDir / "GameModule.vcxproj";
            std::stringstream vcx;
            vcx << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
            vcx << "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
            vcx << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
            vcx << "    <ProjectConfiguration Include=\"Debug|x64\">\n";
            vcx << "      <Configuration>Debug</Configuration>\n";
            vcx << "      <Platform>x64</Platform>\n";
            vcx << "    </ProjectConfiguration>\n";
            vcx << "    <ProjectConfiguration Include=\"Release|x64\">\n";
            vcx << "      <Configuration>Release</Configuration>\n";
            vcx << "      <Platform>x64</Platform>\n";
            vcx << "    </ProjectConfiguration>\n";
            vcx << "  </ItemGroup>\n";
            vcx << "  <PropertyGroup Label=\"Globals\">\n";
            vcx << "    <VCProjectVersion>18.0</VCProjectVersion>\n";
            vcx << "    <Keyword>Win32Proj</Keyword>\n";
            vcx << "    <ProjectGuid>{A34F90D1-857A-413B-9856-115A5E5B9901}</ProjectGuid>\n";
            vcx << "    <RootNamespace>GameModule</RootNamespace>\n";
            vcx << "    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n";
            vcx << "  </PropertyGroup>\n";
            vcx << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
            vcx << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\">\n";
            vcx << "    <ConfigurationType>DynamicLibrary</ConfigurationType>\n";
            vcx << "    <UseDebugLibraries>true</UseDebugLibraries>\n";
            vcx << "    <PlatformToolset Condition=\"'$(DefaultPlatformToolset)' != ''\">$(DefaultPlatformToolset)</PlatformToolset>\n";
            vcx << "    <PlatformToolset Condition=\"'$(DefaultPlatformToolset)' == ''\">v145</PlatformToolset>\n";
            vcx << "    <CharacterSet>Unicode</CharacterSet>\n";
            vcx << "  </PropertyGroup>\n";
            vcx << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\">\n";
            vcx << "    <ConfigurationType>DynamicLibrary</ConfigurationType>\n";
            vcx << "    <UseDebugLibraries>false</UseDebugLibraries>\n";
            vcx << "    <PlatformToolset Condition=\"'$(DefaultPlatformToolset)' != ''\">$(DefaultPlatformToolset)</PlatformToolset>\n";
            vcx << "    <PlatformToolset Condition=\"'$(DefaultPlatformToolset)' == ''\">v145</PlatformToolset>\n";
            vcx << "    <WholeProgramOptimization>true</WholeProgramOptimization>\n";
            vcx << "    <CharacterSet>Unicode</CharacterSet>\n";
            vcx << "  </PropertyGroup>\n";
            vcx << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
            vcx << "  <ImportGroup Label=\"ExtensionSettings\">\n";
            vcx << "  </ImportGroup>\n";
            vcx << "  <ImportGroup Label=\"Shared\">\n";
            vcx << "  </ImportGroup>\n";
            vcx << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n";
            vcx << "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n";
            vcx << "  </ImportGroup>\n";
            vcx << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n";
            vcx << "    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n";
            vcx << "  </ImportGroup>\n";
            vcx << "  <PropertyGroup Label=\"UserMacros\" />\n";
            vcx << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n";
            vcx << "    <OutDir>$(ProjectDir)..\\Binaries\\</OutDir>\n";
            vcx << "    <IntDir>$(ProjectDir)obj\\$(Platform)\\$(Configuration)\\</IntDir>\n";
            vcx << "    <TargetName>GameModule</TargetName>\n";
            vcx << "  </PropertyGroup>\n";
            vcx << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n";
            vcx << "    <OutDir>$(ProjectDir)..\\Binaries\\</OutDir>\n";
            vcx << "    <IntDir>$(ProjectDir)obj\\$(Platform)\\$(Configuration)\\</IntDir>\n";
            vcx << "    <TargetName>GameModule</TargetName>\n";
            vcx << "  </PropertyGroup>\n";

            const std::string commonIncludes = 
                "$(ProjectDir)..\\Assets;$(ProjectDir)..\\Assets\\Scripts;" +
                engineDir.string() + ";" + repoRoot.string() + ";" +
                "C:\\Libraries\\glfw\\include;C:\\Libraries\\glad\\include;C:\\Libraries\\glm;" +
                "C:\\dev\\assimp-6.0.5\\include;C:\\dev\\assimp-6.0.5\\assimp_build\\include;" +
                (engineDir / "Externals").string() + ";" +
                (engineDir / "Externals" / "imgui").string() + ";" +
                (engineDir / "Externals" / "nfd").string() + ";%(AdditionalIncludeDirectories)";

            vcx << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">\n";
            vcx << "    <ClCompile>\n";
            vcx << "      <WarningLevel>Level3</WarningLevel>\n";
            vcx << "      <SDLCheck>true</SDLCheck>\n";
            vcx << "      <PreprocessorDefinitions>_DEBUG;_WINDOWS;_USRDLL;GAMEMODULE_EXPORTS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
            vcx << "      <ConformanceMode>true</ConformanceMode>\n";
            vcx << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
            vcx << "      <AdditionalIncludeDirectories>" << commonIncludes << "</AdditionalIncludeDirectories>\n";
            vcx << "      <DisableSpecificWarnings>4996;%(DisableSpecificWarnings)</DisableSpecificWarnings>\n";
            vcx << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
            vcx << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
            vcx << "    </ClCompile>\n";
            vcx << "    <Link>\n";
            vcx << "      <SubSystem>Windows</SubSystem>\n";
            vcx << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
            vcx << "      <ProgramDatabaseFile>$(OutDir)GameModule_$([System.DateTime]::Now.ToString(\"HHmmssfff\")).pdb</ProgramDatabaseFile>\n";
            vcx << "      <AdditionalDependencies>Engine.lib;glfw3.lib;opengl32.lib;assimp-vc145-mt.lib;%(AdditionalDependencies)</AdditionalDependencies>\n";
            vcx << "      <AdditionalLibraryDirectories>" << debugLibDir.string() << ";C:\\Libraries\\glfw\\lib-vc2022;C:\\dev\\assimp-6.0.5\\assimp_build\\lib\\Release;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
            vcx << "    </Link>\n";
            vcx << "  </ItemDefinitionGroup>\n";

            vcx << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n";
            vcx << "    <ClCompile>\n";
            vcx << "      <WarningLevel>Level3</WarningLevel>\n";
            vcx << "      <FunctionLevelLinking>true</FunctionLevelLinking>\n";
            vcx << "      <IntrinsicFunctions>true</IntrinsicFunctions>\n";
            vcx << "      <SDLCheck>true</SDLCheck>\n";
            vcx << "      <PreprocessorDefinitions>NDEBUG;_WINDOWS;_USRDLL;GAMEMODULE_EXPORTS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
            vcx << "      <ConformanceMode>true</ConformanceMode>\n";
            vcx << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
            vcx << "      <AdditionalIncludeDirectories>" << commonIncludes << "</AdditionalIncludeDirectories>\n";
            vcx << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
            vcx << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
            vcx << "    </ClCompile>\n";
            vcx << "    <Link>\n";
            vcx << "      <SubSystem>Windows</SubSystem>\n";
            vcx << "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n";
            vcx << "      <OptimizeReferences>true</OptimizeReferences>\n";
            vcx << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
            vcx << "      <ProgramDatabaseFile>$(OutDir)GameModule_$([System.DateTime]::Now.ToString(\"HHmmssfff\")).pdb</ProgramDatabaseFile>\n";
            vcx << "      <AdditionalDependencies>Engine.lib;glfw3.lib;opengl32.lib;assimp-vc145-mt.lib;%(AdditionalDependencies)</AdditionalDependencies>\n";
            vcx << "      <AdditionalLibraryDirectories>" << releaseLibDir.string() << ";C:\\Libraries\\glfw\\lib-vc2022;C:\\dev\\assimp-6.0.5\\assimp_build\\lib\\Release;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
            vcx << "    </Link>\n";
            vcx << "  </ItemDefinitionGroup>\n";

            vcx << "  <ItemGroup>\n";
            vcx << "    <ClCompile Include=\"C:\\Libraries\\glad\\src\\glad.c\" />\n";
            for (const auto& f : clCompileFiles) {
                vcx << "    <ClCompile Include=\"" << f << "\" />\n";
            }
            vcx << "  </ItemGroup>\n";

            vcx << "  <ItemGroup>\n";
            for (const auto& f : clIncludeFiles) {
                vcx << "    <ClInclude Include=\"" << f << "\" />\n";
            }
            vcx << "  </ItemGroup>\n";

            vcx << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
            vcx << "  <ImportGroup Label=\"ExtensionTargets\">\n";
            vcx << "  </ImportGroup>\n";
            vcx << "</Project>\n";

            std::string newVcxContent = vcx.str();
            std::string existingVcxContent;
            if (std::filesystem::exists(vcxprojPath)) {
                std::ifstream inVcx(vcxprojPath);
                std::stringstream ssvcx;
                ssvcx << inVcx.rdbuf();
                existingVcxContent = ssvcx.str();
            }

            if (existingVcxContent != newVcxContent) {
                std::ofstream outVcx(vcxprojPath);
                outVcx << newVcxContent;
                outVcx.close();
            }

            // 4. build_module.bat olustur / guncelle
            const std::filesystem::path batPath = projScriptsDir / "build_module.bat";
            std::stringstream bat;
            bat << "@echo off\n";
            bat << "setlocal EnableDelayedExpansion\n";
            bat << "set DEVENV_EXE=C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\devenv.com\n";
            bat << "if not exist \"%DEVENV_EXE%\" (\n";
            bat << "    for /f \"usebackq tokens=*\" %%i in (`\"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -find Common7\\IDE\\devenv.com`) do (\n";
            bat << "        set \"DEVENV_EXE=%%i\"\n";
            bat << "    )\n";
            bat << ")\n";
            bat << "if exist \"!DEVENV_EXE!\" (\n";
            bat << "    \"!DEVENV_EXE!\" \"%~dp0GameModule.vcxproj\" /Build \"Debug|x64\"\n";
            bat << "    exit /b !ERRORLEVEL!\n";
            bat << ")\n\n";
            bat << "set MSBUILD_EXE=C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe\n";
            bat << "if not exist \"%MSBUILD_EXE%\" (\n";
            bat << "    for /f \"usebackq tokens=*\" %%i in (`\"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe`) do (\n";
            bat << "        set \"MSBUILD_EXE=%%i\"\n";
            bat << "    )\n";
            bat << ")\n";
            bat << "if exist \"!MSBUILD_EXE!\" (\n";
            bat << "    \"!MSBUILD_EXE!\" \"%~dp0GameModule.vcxproj\" /p:Configuration=Debug /p:Platform=x64 /nodeReuse:false /m /v:m\n";
            bat << "    exit /b !ERRORLEVEL!\n";
            bat << ")\n\n";
            bat << "echo [ERROR] Neither devenv.com nor MSBuild.exe could be located!\n";
            bat << "exit /b 1\n";

            std::string newBatContent = bat.str();
            std::string existingBatContent;
            if (std::filesystem::exists(batPath)) {
                std::ifstream inBat(batPath);
                std::stringstream ssbat;
                ssbat << inBat.rdbuf();
                existingBatContent = ssbat.str();
            }

            if (existingBatContent != newBatContent) {
                std::ofstream outBat(batPath);
                outBat << newBatContent;
                outBat.close();
            }

            outLog = "Project script scaffolding prepared successfully (" + std::to_string(discoveredScripts.size()) + " script(s) discovered).";
            return true;
        }
        catch (const std::exception& e) {
            outLog = "Exception in ensure_script_scaffolding: " + std::string(e.what());
            return false;
        }
    }

} // namespace project

