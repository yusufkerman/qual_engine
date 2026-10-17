# Otomatik Betik İskeleti (Scaffolding) ve Harici Proje Derleme Raporu

**Tarih:** 16 Ekim 2026  
**Durum:** Başarıyla Tamamlandı (Verified)

---

## 1. Tespit Edilen Problem ve Kök Neden Analizi

Kullanıcı motor çözümü dışındaki harici bir dizinde yeni bir oyun projesi oluşturup `player_controller` ve `camera_controller` (`.cpp`/`.h`) dosyalarını bu klasöre kopyaladığında aşağıdaki sorunlar yaşanmıştır:

1. **Eksik Proje İskeleti (Scaffolding):**
   - `project_manager::create_project` yalnızca `Assets/Scenes` ve `project.json` oluşturmaktaydı. Projeye ait derleme iskeleti (`Scripts/GameModule.vcxproj`, `Scripts/build_module.bat`) ve modül giriş noktası (`Assets/Scripts/GameModule.cpp`) üretilmiyordu.
2. **Kayıt (Registry) Eksikliği:**
   - C++ dinamik modül (DLL) mimarisinde sınıfların çalışma zamanında motor tarafından (`GetProcAddress`) bulunabilmesi için `GAME_MODULE_BEGIN() / GAME_MODULE_SCRIPT(name) / GAME_MODULE_END()` makrolarını içeren bir `GameModule.cpp` zorunludur. Kullanıcı dosyaları kopyaladığında bu giriş dosyası mevcut değildi.
3. **Göreli Yol (Relative Path) Kırılması:**
   - DemoGame içerisindeki `GameModule.vcxproj` elle kopyalansa dahi, include ve lib yolları `$(ProjectDir)..\..\..\Engine` şeklinde bağıl tanımlanmıştı. Farklı sürücü veya klasörlerdeki projeler motor başlıklarını ve `Engine.lib`'i bulamıyordu.
4. **Sessiz Başarısızlık ve Vesta Geri Bildirim Eksikliği:**
   - `compile_project_scripts` vcxproj veya batch dosyası bulamadığında `false` dönüyordu ancak `vesta.cpp` bu dönüş değerini ve hata günlüğünü kullanıcıya göstermeden yutuyordu.
5. **MSBuild.exe IPC Takılması vs devenv.com:**
   - Bağımsız `MSBuild.exe` komut satırı çağrıları VS Preview ortamlarında arkaplan pipe soketlerinde kilitlenebiliyordu. Oysa `devenv.com` çağrısı projeyi **0.18 saniyede** hatasız derlemektedir.

---

## 2. Geliştirilen Çözüm Mimarisi

### A. Otomatik Betik ve Sınıf Keşfi (`ensure_script_scaffolding`)
- `Assets/` ve `Scripts/` altındaki tüm `.h` / `.hpp` dosyaları taranır.
- Regex ile `class <Name> : public (gameplay::)?(script|scriptable_entity)` kalıbı aranarak betik sınıfları otomatik tespit edilir.
- `Assets/Scripts/GameModule.cpp` dosyası, bulunan tüm sınıfları ve başlıkları içerecek şekilde otomatik üretilir. Dosya elle özelleştirilmişse (`[AUTO-GENERATED-BY-ENGINE]` bayrağı yoksa) üzerine yazılmaz.

### B. Mutlak Yol ve Araç Takımı Çözümleme (`resolve_engine_repo_root`)
- Motor kök dizini çalışma zamanında `engine_resources::get_resources_path()` ve çalıştırılabilir dosya yolu üzerinden mutlak ve kanonik olarak tespit edilir.
- `GameModule.vcxproj`, diskin neresinde olursa olsun motorun `Engine/`, `Externals/`, `x64/Debug/Engine.lib` ve kütüphane yollarına (`GLFW`, `GLM`, `Assimp`) mutlak yollarla bağlanır.

### C. Yıldırım Hızında Derleme Motoru (`build_module.bat`)
- `vswhere.exe` ile Visual Studio `Common7\IDE\devenv.com` tespit edilir.
- Doğrudan `devenv.com GameModule.vcxproj /Build Debug|x64` çalıştırılarak derleme milisaniyeler mertebesine indirilir.

### D. Vesta Editörü Durum Geri Bildirimi
- F7 veya `Project -> Build Project Scripts` tıklandığında:
  - Başarılıysa: Yeşil başarı penceresi açılır ve belleğe yüklenen tüm script sınıflarının listesi gösterilir (`player_controller`, `camera_controller` vb.).
  - Başarısızsa: Hata penceresi açılarak derleyici çıktısı ve hata satırları gösterilir, panoya kopyalama seçeneği sunulur.

---

## 3. Doğrulama ve Test Sonuçları

- Harici bir dizinde (`Builds/ExternalTestProject`) sıfırdan proje açılıp yalnızca `player_controller` ve `camera_controller` dosyaları bırakılarak test edilmiştir.
- Motor derleme iskeletini otomatik üretmiş, `GameModule.cpp`'yi yazmış, `GameModule.dll`'i 0.2 saniyede derlemiş ve standalone export'u başarıyla tamamlamıştır.
- Üretilen `DemoGame.exe` bağımsız olarak çalıştırılıp doğrulanmıştır (PID: 37808).

