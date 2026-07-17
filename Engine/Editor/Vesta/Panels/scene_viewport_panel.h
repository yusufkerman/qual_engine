#pragma once

#include "IEditorPanel.h"
#include "../ViewModels/scene_viewport_view_model.h"
#include "../CoreState/editor_viewport_state.h"

namespace vesta {

    class scene_viewport_panel : public IEditorPanel
    {
    public:

        scene_viewport_panel(
            scene_viewport_view_model& vm,
            editor_viewport_state& viewportState)
            :
            m_vm(vm),
            m_viewport_state(viewportState)
        {
        }

        void render() override;

        const std::string& get_title() const override { return m_title; }

        bool is_open() const override { return m_is_open; }

        void set_open(bool open) override { m_is_open = open; }

    private:

        scene_viewport_view_model& m_vm;
        editor_viewport_state& m_viewport_state;

        std::string m_title = "Scene";
        bool m_is_open = true;
    };

}