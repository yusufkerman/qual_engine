# Aşama 2: Application Katmanının Temizlenmesi ve GameRuntime Temelleri Uygulama Raporu

> **Tarih:** 16 Ekim 2026  
> **Aşama:** 2 / 4  
> **Durum:** ✅ Tamamlandı  
> **İlgili Yol Haritası:** [01_architecture_and_roadmap.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/01_architecture_and_roadmap.md)

---

## 1. Amaç ve Kapsam

Bu aşamanın temel amacı, `Application` projesinin "hem editör hem de demo oyun" olan çift kimliğini sonlandırmak; demo oyun scriptlerini (`player_controller`, `camera_controller`, `sandbox.cpp`) motor çekirdeğinden ve editör projesinden söküp bağımsız bir proje dizinine (`Projects/DemoGame/`) taşımaktır.

Böylece:
1. `Application.exe` sadece **Vesta Editör Başlatıcısı (`EditorLauncher`)** olarak çalışır.
2. Editör açıldığında sahnede zorla hardcoded zemin, küp oyuncu ve ışık oluşturan `sandbox_load_demo_game` çağrısı engellenir; yalnızca açılan projenin sahnesi yüklenir.
3. `script_factory` statik kayıt kirliliğinden arındırılır; bir nesneye script eklerken Inspector'da sabit olarak demo scriptleri listelenmez.
4. İleride dağıtılacak oyun çıktısı (Standalone Game) için editörsüz çalışan saf yürütücü modülü [`game_runtime`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Runtime/game_runtime.h) sisteme kazandırılır.

---

## 2. Gerçekleştirilen Mimari Değişiklikler

### 2.1. `Application` Projesinin Saf EditorLauncher'a Dönüştürülmesi

* **Demo Kodların Çıkarılması:**
  [`Application/Application.vcxproj`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Application/Application.vcxproj) ve `.filters` dosyasından `sandbox.cpp`, `sandbox.h`, `player_controller.cpp/h`, `camera_controller.cpp/h` girdileri kaldırıldı.
* **Hafif ve Temiz Giriş Noktası:**
  [`Application/Application.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Application/Application.cpp) yeniden yazılarak doğrudan `argc, argv` desteğine kavuşturuldu. Komut satırından veya dosya yöneticisinden bir proje dosyası (`project.json`) verildiğinde doğrudan o projeyi açar; verilmediğinde varsayılan proje oturumuyla ayağa kalkar.
* **Kazanım:** `Application.exe` derleme süresi 2 saniyeye inmiştir ve içinde sıfır oyun kodu barındırır.

---

### 2.2. İzole Demo Projesinin Kurulması (`Projects/DemoGame/`)

Tüm demo varlıkları ve scriptleri motor çekirdeğinden çıkarılıp bağımsız bir kullanıcı projesi formatında yapılandırıldı:
```
Projects/DemoGame/
├── project.json                    (Pencere, render ve ışık yapılandırması)
├── sandbox_reference.cpp           (Eski sandbox sahne kurma referansı)
├── sandbox_reference.h
├── Binaries/                       (Derlenecek GameModule.dll için)
└── Assets/
    ├── Scenes/                     (Main.json başlangıç sahnesi)
    └── Scripts/
        ├── player_controller.h
        ├── player_controller.cpp
        ├── camera_controller.h
        └── camera_controller.cpp
```

---

### 2.3. Dinamik Script Fabrikası Temizliği (`script_factory`)

[`Engine/Engine/Scripting/script_factory.h`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Engine/Scripting/script_factory.h) sınıfına dinamik kayıt yönetimi fonksiyonları eklendi:
- `unregister_script(name)`
- `unregister_all_scripts()`

Artık `Application.exe` açıldığında `script_factory::get_all_script_names()` tamamen temiz (boş) başlar; başka projelerin veya silinmiş sınıfların Inspector paneline sızması engellendi.

---

### 2.4. Standalone Player Modülü (`game_runtime`)

Editörden bağımsız, son kullanıcı için üretilecek tek parça oyun yürütücüsü temeli atıldı:
- [`Engine/Runtime/game_runtime.h`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Runtime/game_runtime.h)
- [`Engine/Runtime/game_runtime.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Runtime/game_runtime.cpp)

Bu modül:
1. `boot.config` dosyasını okuyarak ekran çözünürlüğü, başlangıç sahnesi ve proje adını yükler.
2. Motoru `isEditorBuild = false` modunda (ImGui panelleri ve editör katmanı olmadan) başlatır.
3. Başlangıç sahnesini doğrudan yükleyip `PLAY` moduna geçirir ve tam ekran oyun döngüsünü yürütür.

---

### 2.5. Skybox Yedekleme ve Dahili Kaynak Entegrasyonu

[`Engine/Engine/engine.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Engine/engine.cpp) içinde sabit `Assets/sky/skybox1.png` yolu yerine, Phase 1'de ayrıştırdığımız `@engine/Textures/sky/skybox1.png` dahili kaynak yolu bağlandı. Kullanıcı sıfır varlıklı boş bir proje açsa bile grafik boru hattı çökmeden çalışmaya devam eder.

---

## 3. Doğrulama ve Test Sonuçları

1. **Derleme Testi:**
   - `Engine.vcxproj` -> `Engine.lib` ✅ (0 Hata)
   - `Application.vcxproj` -> `Application.exe` ✅ (0 Hata, ~2 saniye derleme süresi)
2. **Çalıştırma Testi:**
   - `Application.exe` çalıştırıldı ve 3 saniye boyunca hiçbir çökme olmadan stabil çalıştığı doğrulandı.
   - `Application.exe Projects\DemoGame\project.json` komut satırı argümanı ile çalıştırıldı; proje başarıyla açıldı ve test edildi.
3. **Inspector İzolasyonu:**
   - `script_factory` içinde statik macro kaydı kalmadığı için `Script Class` combo box'ında motorun içine gömülü demo scriptleri çıkmamaktadır.

---

## 4. Sonraki Adım: Aşama 3 (Projeye Özel Scripting ve GameModule.dll)

1. Proje scriptlerinin (`Projects/DemoGame/Assets/Scripts/`) motor dışından bağımsız bir `GameModule.dll` olarak derlenmesi için altyapı hazırlanması.
2. Editörün açılan projenin `Binaries/GameModule.dll` dosyasını `LoadLibrary` ile dinamik olarak bağlaması ve Inspector'da yalnızca o projenin scriptlerini listelemesi.

