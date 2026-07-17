#pragma once
#include <glm/glm.hpp>
#include "../CoreState/editor_viewport_state.h"


namespace vesta {

    class scene_viewport_view_model {
    public:
        explicit scene_viewport_view_model(editor_viewport_state& state)
            : m_state(state)
        {}

        unsigned int get_texture_id() const { return m_texture_id; }
        void set_texture_id(unsigned int id) { m_texture_id = id; }

        const glm::vec2& get_size() const { return m_size; }
        void set_size(const glm::vec2& size) { m_size = size; }

        bool is_focused() const { return m_is_focused; }
        void set_focused(bool focused) { m_is_focused = focused; }

        bool is_hovered() const { return m_is_hovered; }
        void set_hovered(bool hovered) { m_is_hovered = hovered; }

    private:
        editor_viewport_state& m_state;
        unsigned int m_texture_id{ 0 };
        glm::vec2 m_size{ 800.0f, 600.0f };
        bool m_is_focused{ false };
        bool m_is_hovered{ false };
    };

} // namespace vesta