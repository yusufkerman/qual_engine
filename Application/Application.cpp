/**
 * @file Application.cpp
 * @brief Vesta Editör Başlatıcısı (EditorLauncher).
 * @note Bu katman hiçbir demo veya oyun betiği barındırmaz; yalnızca Vesta Editör ortamını ilklendirir
 * ve aktif kullanıcı projesini yönetir.
 */

#include <Engine/engine.h>
#include <Editor/editor_context.h>
#include <Project/project.h>
#include <Project/project_exporter.h>
#include <iostream>
#include <cstring>

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;

    // Motor ve Editör çalışma zamanı bağlamları (Engine & Editor Contexts)
    EngineContext ctx;
    EditorContext editorCtx;

    ctx.isEditorBuild = true;

    // 1. OpenGL, GLFW, ImGui, Vesta ve Aria sistemlerini başlat
    if (!Engine::init(ctx, editorCtx))
    {
        std::cerr << "[EditorLauncher] Hata: Engine::init basarisiz oldu.\n";
        return -1;
    }

    // Headless CLI export desteği: Application.exe --export <project.json> <output_folder>
    if (argc >= 4 && std::string(argv[1]) == "--export")
    {
        std::cout << "[EditorLauncher] Headless Export Modu: " << argv[2] << " -> " << argv[3] << "\n";
        project::project_manager::instance().open_project(argv[2], ctx);

        project::export_options opt;
        opt.output_directory = argv[3];
        opt.compile_scripts = true;
        opt.copy_engine_resources = true;
        opt.copy_dependencies = true;

        auto res = project::project_exporter::export_project(opt);
        for (const auto& log : res.log_messages) {
            std::cout << log << "\n";
        }

        if (res.success) {
            std::cout << "[EditorLauncher] Export Basarili: " << res.target_executable << "\n";
        } else {
            std::cerr << "[EditorLauncher] Export Hatasi: " << res.error_message << "\n";
        }

        std::_Exit(res.success ? 0 : 1);
    }

    // 2. Komut satırından proje yolu verildiyse o projeyi aç
    if (argc > 1 && argv[1] != nullptr && std::strlen(argv[1]) > 0)
    {
        std::cout << "[EditorLauncher] Komut satirindan proje aciliyor: " << argv[1] << "\n";
        project::project_manager::instance().open_project(argv[1], ctx);
    }

    // 3. Ana Editör Döngüsü
    while (!glfwWindowShouldClose(ctx.window))
    {
        // Girdiler, sahne ve editör panelleri güncellenir
        Engine::update(ctx, editorCtx);

        // Editör ve oyun viewportları render edilir
        Engine::render(ctx, editorCtx);
    }

    // 4. Motor ve kaynaklar temizlenir
    Engine::shutdown(ctx, editorCtx);

    return 0;
}