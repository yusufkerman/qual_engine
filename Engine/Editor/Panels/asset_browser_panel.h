#pragma once

#include "../asset_browser_system.h" // Bir önceki aşamada yazdığımız std::filesystem tarayıcısı
#include "../Engine/engine_context.h" // Projendeki context yapısı
#include <string>

// 🚀 Aria system'ı doğrudan include etmeden pointer olarak kullanabilmek için ön bildirim
namespace aria {
    class system;
}

namespace aria::editor {

    class asset_browser_panel {
    private:
        asset_browser_system m_browser_system;
        std::string m_selected_path; // O an tıklanan dosyanın yolu

        // Hiyerarşik ağacı ekrana recursive (özyinelemeli) çizen iç fonksiyon
        void draw_node(const asset_file_entry& entry, EditorContext& ctx);
        
    public:
        // 🚀 Kurucu fonksiyona opsiyonel aria::system* enjeksiyonu eklendi
        asset_browser_panel(aria::system* aria_sys = nullptr);
        ~asset_browser_panel() = default;

        // Panel ilklendirilirken diski bir kez tarar
        void init();

        // Editor::renderUI içerisinden çağrılacak ana render fonksiyonu
        void render(EditorContext& ctx);

        [[nodiscard]] std::string get_selected_path() const { return m_selected_path; }
        void clear_selection() { m_selected_path.clear(); }
    };

} // namespace aria::editor