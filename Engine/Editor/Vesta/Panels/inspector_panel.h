#pragma once

#include "IEditorPanel.h"
#include "../ViewModels/inspector_view_model.h"

#include <memory>
#include <string>

namespace vesta
{

    class inspector_panel final : public IEditorPanel
    {
    private:

        std::unique_ptr<inspector_view_model> m_vm;

        std::string m_title = "Inspector";
        bool m_is_open = true;

    public:

        explicit inspector_panel(
            std::unique_ptr<inspector_view_model> vm);

        ~inspector_panel() override;

        void render() override;

        const std::string& get_title() const override
        {
            return m_title;
        }

        bool is_open() const override
        {
            return m_is_open;
        }

        void set_open(bool open) override
        {
            m_is_open = open;
        }
    };

}