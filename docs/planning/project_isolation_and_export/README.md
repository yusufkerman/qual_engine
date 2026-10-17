# Proje İzolasyonu, Dinamik Scripting ve Standalone Export Sistemi

Bu dizin, motorun monolitik yapısından tam izole proje modeline geçiş sürecini, mimari tasarım kararlarını ve her aşamanın uygulama raporlarını içerir.

## Dizin İçeriği

1. [01_architecture_and_roadmap.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/01_architecture_and_roadmap.md)
   - Ana mimari tasarım raporu
   - Unreal Engine / Hazel karşılaştırmaları
   - 4 Aşamalı uygulama yol haritası
2. [02_phase1_builtin_resources_report.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/02_phase1_builtin_resources_report.md) ✅ *(Tamamlandı)*
   - Dahili motor kaynaklarının (`Engine/Resources/Shaders/`, dokular vb.) ayrıştırılması, `@engine/` sanal yolları ve sabit GUID eşleştirmeleri.
3. [03_phase2_application_decoupling_report.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/03_phase2_application_decoupling_report.md) ✅ *(Tamamlandı)*
   - `Application` projesinin EditorLauncher olarak sadeleştirilmesi, demo kodların izole projeye (`Projects/DemoGame/`) taşınması ve `game_runtime` temelleri.
4. [04_phase3_dynamic_scripting_report.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/04_phase3_dynamic_scripting_report.md) ✅ *(Tamamlandı)*
   - `GameModule.dll` dinamik yükleme (`LoadLibrary`), gölge kopyalama (`GameModule_runtime.dll`), C-linkage export ve projeye özel script listeleme.
5. [05_phase4_export_pipeline_report.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/05_phase4_export_pipeline_report.md) ✅ *(Tamamlandı)*
   - Standalone player (`GameRuntime.exe`), `project_exporter`, Vesta `Project` menüsü, `boot.config` ve tek tıkla çalıştırılabilir oyun paketi çıktısı üretimi.


