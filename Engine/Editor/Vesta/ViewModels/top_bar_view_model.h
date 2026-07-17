#pragma once

#include <string>
#include <functional>

#include "../../Editor/editor_context.h"

namespace vesta
{

    class top_bar_view_model
    {
    public:

        using FileActionCallback =
            std::function<void(const std::string&)>;


        top_bar_view_model(
            EditorContext& editorCtx,
            FileActionCallback onSaveRequested,
            FileActionCallback onLoadRequested)
            :
            m_editor_ctx(editorCtx),
            m_onSaveRequested(onSaveRequested),
            m_onLoadRequested(onLoadRequested)
        {
        }


        EngineMode get_mode() const
        {
            return m_editor_ctx.mode;
        }


        void toggle_mode()
        {
            m_editor_ctx.mode =
                (m_editor_ctx.mode == EngineMode::EDITOR)
                ? EngineMode::PLAY
                : EngineMode::EDITOR;
        }


        void execute_save(const std::string& path)
        {
            if (m_onSaveRequested)
                m_onSaveRequested(path);
        }


        void execute_load(const std::string& path)
        {
            if (m_onLoadRequested)
                m_onLoadRequested(path);
        }


    private:

        EditorContext& m_editor_ctx;

        FileActionCallback m_onSaveRequested;
        FileActionCallback m_onLoadRequested;

    };

}