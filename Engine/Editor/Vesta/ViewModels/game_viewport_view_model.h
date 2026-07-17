#pragma once
#include <glm/glm.hpp>
#include "../CoreState/editor_viewport_state.h"


namespace vesta {

    class game_viewport_view_model {
    public:
        explicit game_viewport_view_model(editor_viewport_state& state)
            : m_state(state)
        {}

        // Getters / Setters
        unsigned int get_texture_id() const { return m_texture_id; }
        void set_texture_id(unsigned int id) { m_texture_id = id; }

        const glm::vec2& get_size() const { return m_size; }
        void set_size(const glm::vec2& size) { m_size = size; }

        void set_focused(bool focused) { m_is_focused = focused; }
        bool is_focused() const { return m_is_focused; }

        void set_hovered(bool hovered) { m_is_hovered = hovered; }
        bool is_hovered() const { return m_is_hovered; }

        void set_position(const glm::vec2& pos) { m_position = pos; }
        const glm::vec2& get_position() const { return m_position; }

    private:
        editor_viewport_state& m_state;
        unsigned int m_texture_id{ 0 };
        glm::vec2 m_size{ 800.0f, 600.0f };
        glm::vec2 m_position{ 0.0f, 0.0f };
        bool m_is_focused{ false };
        bool m_is_hovered{ false };
    };

} // namespace vesta