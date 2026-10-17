# Aşama 1: Built-in Shader ve Motor Kaynakları Ayrımı Uygulama Raporu

> **Tarih:** 16 Ekim 2026  
> **Aşama:** 1 / 4  
> **Durum:** ✅ Tamamlandı  
> **İlgili Yol Haritası:** [01_architecture_and_roadmap.md](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/project_isolation_and_export/01_architecture_and_roadmap.md)

---

## 1. Amaç ve Kapsam

Bu aşamanın temel hedefi, motorun çalışabilmesi için zorunlu olan dahili shader'ları (`default_shader`, `skinned_default`, `gizmo`, `grid`, `skybox`, `shadow`, `shadow_skinned`) ve dahili primitif mesh'leri, kullanıcının proje dizininden (`Assets/`) ve `Application` klasöründen tamamen soyutlamaktır.

Bu sayede:
1. Kullanıcı yeni veya boş bir proje açtığında motor dahili shader'ları eksiksiz yüklenir.
2. Kullanıcının projesine motor çekirdeğine ait `.vert`, `.frag` veya `.meta` dosyaları bulaşmaz.
3. Materyaller, deterministik ve sabit GUID'ler üzerinden motor dahili shader'larına bağlanır.

---

## 2. Yapılan Mimari Değişiklikler ve Yenilikler

### 2.1. Motor Dahili Kaynaklar Klasörü (`Engine/Resources/`)

Motor kökünde bağımsız bir dahili kaynak dizini oluşturuldu:
```
OpenGL-Learn-1/
├── Engine/
│   └── Resources/
│       ├── Shaders/
│       │   ├── default_shader.vert / .frag / .meta
│       │   ├── skinned_default.vert / .frag / .meta
│       │   ├── shadow.vert / .frag / .meta
│       │   ├── shadow_skinned.vert / .frag / .meta
│       │   ├── Debug/
│       │   │   ├── gizmo.vert / .frag / .meta
│       │   │   └── grid.vert / .frag / .meta
│       │   └── Skybox/
│       │       └── skybox.vert / .frag / .meta
│       └── Textures/
```

---

### 2.2. Motor Dahili Kaynak Yöneticisi (`engine_resources`)

[`Engine/Resources/engine_resources.h`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Resources/engine_resources.h) ve [`Engine/Resources/engine_resources.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Resources/engine_resources.cpp) modülleri geliştirildi:

1. **Dinamik Kök Dizin Tespiti (`detect_engine_resources_path`):**
   - **Geliştirme / Editör Modu:** Çalıştırılabilir dosyanın (`Application.exe`) veya çalışma dizininin göreli yollarından otomatik olarak `Engine/Resources/` dizinini bulur (`..\..\Engine\Resources`).
   - **Standalone / Export Modu:** İleride paketlenecek bağımsız oyun çıktılarında çalıştırılabilir dosyanın yanındaki `EngineResources/` veya `Resources/` klasörünü otomatik tanır.
2. **Deterministik ve Sabit GUID Eşleştirmesi:**
   Rastgele GUID üretiminden kaynaklanan uyumsuzlukları önlemek için motor dahili varlıklarına sabit kimlikler verildi:
   | Varlık | Sanal Yol (Virtual Path) | Sabit GUID |
   | :--- | :--- | :--- |
   | Default Shader | `@engine/Shaders/default_shader` | `c10d2850-634a-4e41-bd53-add2e817e129` |
   | Skinned Shader | `@engine/Shaders/skinned_default` | `cd5743f9-dbda-4c13-8a08-c41f6c9bfa71` |
   | Gizmo Shader | `@engine/Shaders/Debug/gizmo` | `1a96340a-1192-4f27-b8fd-0439b686f63b` |
   | Grid Shader | `@engine/Shaders/Debug/grid` | `ca6418ad-3833-40aa-9847-5bbc212a890a` |
   | Skybox Shader | `@engine/Shaders/Skybox/skybox` | `ab217918-3c47-43c5-b01e-e4a5feae6a82` |
   | Shadow Shader | `@engine/Shaders/shadow` | `dab2ae4c-56df-4dcb-a781-82fabfacee3d` |
   | Shadow Skinned | `@engine/Shaders/shadow_skinned` | `e5c2194b-4b19-4824-a749-33b684cb32fa` |
   | Cube Mesh | `engine://builtin/cube` | `d23a0aff-3fdc-5851-a6c3-e9623da3cd11` |
   | Sphere Mesh | `engine://builtin/sphere` | `f18b3e21-9541-4770-bc2d-5b3a8829f001` |
3. **Otomatik Kayıt (`register_builtin_resources`):**
   Motor açılışında tüm bu dahili kaynaklar Aria varlık sistemine kalıcı olarak kaydedilir; böylece kullanıcının `asset_manifest.json` dosyasına yazılması gerekmez.

---

### 2.3. Varlık Boru Hattı Güncellemeleri

1. [`Engine/Engine/asset_pipeline.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Engine/asset_pipeline.cpp):
   - `get_or_create_meta_guid`: Belirtilen yol motor dahili kaynağı ise doğrudan sabit GUID'i döndürür, diske sahte `.meta` dosyaları oluşturulmasını engeller.
   - `import_shader`: Yolu `engine_resources::resolve_path` üzerinden çözerek Aria sistemine aktarır.
2. [`Engine/Engine/Assets/asset_loader.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Engine/Assets/asset_loader.cpp):
   - `load_shader_to_cpu`: Gelen dosya yolunu `engine_resources::resolve_path` ile çözümler; böylece `@engine/Shaders/...` veya `Shaders/...` yolları her çalışma ortamında doğru fiziksel `.vert` ve `.frag` dosyalarına ulaşır.
3. [`Engine/Editor/Vesta/Services/material_service.cpp`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Editor/Vesta/Services/material_service.cpp):
   - Yeni materyal oluşturulurken varsayılan shader GUID'i olarak doğrudan `engine_resources::builtin_guid::default_shader` atanır.
4. [`Engine/Editor/Vesta/Inspector/Assets/material_inspector.h`](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Engine/Editor/Vesta/Inspector/Assets/material_inspector.h):
   - Materyal ayrıntılarında shader yolu temiz sanal gösterim olan `@engine/Shaders/default_shader` olarak gösterilir.

---

### 2.4. Derleme ve Araç Zinciri İyileştirmeleri (Build Fix)

* **Problem:** 32-bit `MSBuild.exe` PowerShell üzerinden çağrıldığında standart konsol giriş kuyruğunda (`stdin`) kilitleniyor ve derleme ilerlemiyordu.
* **Çözüm:**
  - Derleyici 64-bit yerel derleyiciye (`Bin/amd64/MSBuild.exe`) geçirildi.
  - Çoklu işlemci paralel derleme bayrakları (`/p:MultiProcessorCompilation=true /m`) etkinleştirildi.
  - `.vscode/tasks.json` doğrudan `amd64/MSBuild.exe` çağıracak şekilde optimize edildi.
  - Tüm motor ve uygulama 20 saniye altında sıfır hata ile derlendi.

---

## 3. Doğrulama ve Test Sonuçları

1. **Derleme Testi:**
   - `Engine.vcxproj` -> `Engine.lib` ✅ (0 Hata)
   - `Application.vcxproj` -> `Application.exe` ✅ (0 Hata)
2. **Geriye Dönük Uyumluluk:**
   Mevcut sahnelerde ve materyallerde kayıtlı olan eski shader GUID'leri korunduğu için hiçbir sahne veya materyal bozulmadı.

---

## 4. Sonraki Adım: Aşama 2 (Application Katmanının Temizlenmesi)

1. `Application` projesinin yalnızca editörü başlatan saf bir `EditorLauncher` haline getirilmesi.
2. Demo oyun kodlarının (`sandbox.cpp`, `player_controller.h/cpp`, `camera_controller.h/cpp`) ayrıştırılıp izole bir örnek projeye (`Projects/DemoGame/`) taşınması.

