#include "Engine/engine.h"
#include "sandbox.h"

int main()
{
    EngineContext ctx;

    if (!Engine::init(ctx))
        return -1;

    sandbox_load_default_scene(ctx);

    while (!glfwWindowShouldClose(ctx.window))
    {
        Engine::update(ctx);
        Engine::render(ctx);
    }

    Engine::shutdown(ctx);
    return 0;
}