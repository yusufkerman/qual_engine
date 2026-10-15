# Gameplay API ve Betik Soyutlama Katmanı Raporları

Bu dizin, motorun çekirdek yapısı ile oyun betikleri (native script) arasına yerleştirilen **Gameplay API** katmanının mimari analiz, tasarım, yol haritası ve uygulama raporlarını barındırır.

---

## 📑 Rapor Listesi

1. **[01. Mevcut Betik ve Motor Etkileşim Durumu Analizi](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/gameplay_api/01_current_state_analysis.md)**
   - Mevcut `scriptable_entity`, `player_controller`, `camera_controller` kodlarının taranması.
   - Doğrudan `EngineContext` ve ham ECS işaretçisi sızıntıları.
   - Jolt fizik entegrasyonu ve temas dinleyicisi (`ContactListener`) eksikliğinin tespiti.

2. **[02. Önerilen Gameplay API Mimari Tasarımı](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/gameplay_api/02_gameplay_api_architecture.md)**
   - Katmanlı mimari (Kullanıcı Betikleri -> Gameplay API -> Çekirdek Motor).
   - Yeni yaşam döngüsü (`on_fixed_update`, `on_collision_enter/exit`, `on_trigger_enter/exit`).
   - Thread-safe Jolt Contact Listener ve Çarpışma Olay Kuyruğu (Queue & Dispatch) yapısı.
   - `gameplay::input`, `transform_wrapper`, `rigidbody_wrapper` tasarım detayları.

3. **[03. Aşamalı Uygulama ve Geçiş Stratejisi (Roadmap)](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/gameplay_api/03_roadmap_and_migration_strategy.md)**
   - Mevcut projeyi bozmadan (Zero Breaking Changes) aşamalı geçiş planı.
   - 4 Fazlık yol haritası (Fizik Olayları -> API Servisleri -> Yaşam Döngüsü Bağlantısı -> Doğrulama).

4. **[04. Gameplay API Modülü Geliştirme ve Tamamlanma Raporu](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/gameplay_api/04_gameplay_api_implementation_report.md)**
   - Modülün geliştirilen tüm bileşenleri (`Engine/Gameplay/`).
   - Girdi, Dönüşüm, Fizik, Varlık, Sahne ve Betik API'lerinin detayları.
   - Eski ve yeni kod geliştirici deneyimi karşılaştırması.
   - Derleme ve geriye dönük uyumluluk doğrulama sonuçları.
