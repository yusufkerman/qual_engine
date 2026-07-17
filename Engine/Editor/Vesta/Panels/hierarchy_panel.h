#pragma once
#include "IEditorPanel.h"
#include "../ViewModels/hierarchy_view_model.h"
#include <memory>


namespace vesta {

    class hierarchy_panel : public IEditorPanel {
    private:
        std::unique_ptr<hierarchy_view_model> m_vm;
        std::string m_title = "Hierarchy";
        bool m_is_open = true;

    public:
        hierarchy_panel(IEditableWorld& world, registry& reg, editor_selection_manager& selection)
            : m_vm(std::make_unique<hierarchy_view_model>(world, reg, selection)) {
        }

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }
    };

}