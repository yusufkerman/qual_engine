#include "sera.h"
#include "../Engine/engine_context.h"


namespace sera {

    void sera::init(EngineContext& ctx)
    {
        m_sera_session = std::make_unique<sera_session>(ctx);
    }

    validation_report sera::save_scene(
        const std::string& filepath,
        const scene& current_scene)
    {
        if (!m_sera_session)
        {
            validation_report report;
            report.add_critical("SERA is not initialized.");
            return report;
        }

        return m_sera_session->save(filepath, current_scene);
    }

    validation_report sera::load_scene(
        const std::string& filepath,
        scene& out_current_scene)
    {
        if (!m_sera_session)
        {
            validation_report report;
            report.add_critical("SERA is not initialized.");
            return report;
        }

        return m_sera_session->load(filepath, out_current_scene);
    }

} // namespace sera