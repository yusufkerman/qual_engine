#include "Engine/engine.h"

int main()
{
    EngineContext ctx;

    if (!Engine::init(ctx))
        return -1;

    while (!glfwWindowShouldClose(ctx.window))
    {
        Engine::update(ctx);
        Engine::render(ctx);
    }

    Engine::shutdown(ctx);
    return 0;
}