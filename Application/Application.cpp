#include <Engine/engine.h>
#include "sandbox.h"
#include <iostream>
#include <Scene/scene_serializer.h>

int main()
{
    // Motorun çalışması için gereken tüm global/runtime state burada tutulur.
    // Window, renderer, input, scene session, editor vb. sistemler bu context içindedir.
    EngineContext ctx;

    ctx.isEditorBuild = true;

    // OpenGL, GLFW, ImGui, shaderlar ve render target'lar oluşturulur.
    // Başarısız olursa program sonlandırılır.
    if (!Engine::init(ctx))
        return -1;

    // Varsayılan test/sandbox sahnesini yükler.
    // Önemli:
    // Motor artık hangi modeli veya sahneyi yükleyeceğini bilmez.
    // İçerik oluşturma sorumluluğu Sandbox katmanındadır.
    sandbox_load_scene2_directly(ctx);
    std::cout << "mat size: " << ctx.materialManager.get_all_materials().size() << "\n";

    bool sceneLoaded = false;

  
    if (!ctx.isEditorBuild)
    {
        ctx.session.play();
        ctx.editor.mode = EngineMode::PLAY;
        ctx.prevMode = EngineMode::EDITOR;
    }

    // Ana oyun döngüsü
    while (!glfwWindowShouldClose(ctx.window))
    {

        // Input, editor güncellemeleri,
        // play/editor geçişleri ve runtime mantıkları çalıştırılır.
        Engine::update(ctx);

        // Editor viewport, game viewport ve UI render edilir.
        Engine::render(ctx);
    }

    // GPU kaynakları, render target'lar,
    // ImGui ve pencere sistemi temizlenir.
    Engine::shutdown(ctx);

    return 0;
}