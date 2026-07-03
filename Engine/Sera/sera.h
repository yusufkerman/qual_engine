#pragma once

#include "sera_session.h"
#include "../Scene/scene_session.h"

#include <memory>
#include <string>

struct EngineContext;

namespace sera {

    class sera
    {
    public:
        void init(EngineContext& ctx);

        validation_report save_scene(
            const std::string& filepath,
            const scene& current_scene);

        validation_report load_scene(
            const std::string& filepath,
            scene& out_current_scene);

    private:
        std::unique_ptr<sera_session> m_sera_session;
    };

} // namespace sera