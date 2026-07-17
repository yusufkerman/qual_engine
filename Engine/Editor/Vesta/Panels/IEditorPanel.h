#pragma once
#include <string>

namespace vesta {

    class IEditorPanel {
    public:
        virtual ~IEditorPanel() = default;

        virtual void render() = 0; // ImGui çizim fonksiyonu
        virtual const std::string& get_title() const = 0;
        virtual bool is_open() const = 0;
        virtual void set_open(bool open) = 0;
    };

} // namespace aria::editor