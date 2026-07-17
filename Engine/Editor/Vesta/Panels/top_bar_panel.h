#pragma once
#include "IEditorPanel.h"
#include "../ViewModels/top_bar_view_model.h"

namespace vesta {

    class top_bar_panel : public IEditorPanel {
    public:
        explicit top_bar_panel(top_bar_view_model& vm) : m_vm(vm) {}

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        top_bar_view_model& m_vm;
        std::string m_title = "TopBar";
        bool m_is_open = true;
    };

} // namespace vesta