# Rapor 1: Mevcut Betik (Scripting) ve Motor Etkileşim Durumu Analizi

## 1. Giriş ve Amaç
Bu rapor, projede şu anda yer alan C++ yerel betik (native script) sisteminin, oyun motorunun çekirdek bileşenleriyle (`EngineContext`, `registry`, `physics_system`, `InputState`) nasıl etkileşime girdiğini incelemek ve gelecekte kurulacak **Gameplay API** katmanı için mevcut yapısal eksiklikleri tespit etmek amacıyla hazırlanmıştır.

---

## 2. Mevcut Kod Mimarisi Taraması

### 2.1. Temel Betik Sınıfı (`scriptable_entity`)
Dosya: `Engine/Scene/scriptable_entity.h`
```cpp
class scriptable_entity {
public:
    virtual void on_create() {}
    virtual void on_destroy() {}
    virtual void on_update(float delta_time) {}

    EngineContext& getContext() const { return *ctx; }
    registry* get_registry() const { return m_registry; }
    template<typename T> T& get_component();
    entity find_by_name(const std::string& name);
private:
    entity m_entity = 0;
    registry* m_registry = nullptr;
    EngineContext* ctx = nullptr;
};
```

#### Tespit Edilen Zayıflıklar ve Riskler:
1. **İç Yapı Sızıntısı (`EngineContext` Leak):** 
   - `getContext()` doğrudan tüm motor çekirdeğini açmaktadır. Bir betik geliştiricisi pencere referansına (`GLFWwindow*`), iç grafik kaynaklarına, render pass'lerine ve editör durumlarına kadar erişebilmektedir.
   - Bu durum, betik ile motor arasındaki sınırları silmekte; geliştiricinin motora yanlışlıkla müdahale etmesine ve sistemlerin sıkı sıkıya birbirine bağlanmasına (tight coupling) neden olmaktadır.
2. **Kısıtlı Yaşam Döngüsü (Lifecycle):**
   - Yalnızca `on_create`, `on_destroy` ve `on_update` mevcuttur.
   - Fizik simülasyonuyla senkron çalışan bir `on_fixed_update(float fixed_dt)` yoktur.
   - Çarpışma ve temas olayları (`on_collision_enter`, `on_collision_exit`, `on_trigger_enter`, `on_trigger_exit`) mevcut değildir.
3. **Ham ECS İşaretçisi Bağımlılığı (`registry*`):**
   - Varlık sorgulama ve bileşen okuma işlemleri doğrudan ECS `registry` işaretçisi üzerinden yapılmaktadır. Bileşen varlığı kontrol edilmediğinde veya silinmiş bir varlığa erişildiğinde doğrudan tanımsız davranış (undefined behavior) veya çökme riski oluşmaktadır.

---

### 2.2. Mevcut Örnek Betiklerin İncelenmesi

#### A. `player_controller.cpp` Analizi
Dosya: `Application/Assets/Scripts/player_controller.cpp`
* **Girdi Erişimi:**
  ```cpp
  auto& input = getContext().inputState;
  if (input.actions[(int)InputAction::MoveForward]) moveDir += forward;
  ```
  *Sorun:* Betik doğrudan `inputState` struct'ının ham dizisini okumaktadır. Tuşun yeni mi basıldığı (`just_pressed`), basılı mı tutulduğu (`is_down`), yoksa bırakıldığı mı (`just_released`) gibi yüksek seviye sorgular yoktur.
* **Fizik Erişimi:**
  ```cpp
  auto& physics = getContext().physicsSystem.api();
  glm::vec3 currentVel = physics.get_velocity(get_entity());
  physics.set_velocity(get_entity(), finalVel);
  ```
  *Sorun:* Betik, `EngineContext` içinden `physicsSystem` nesnesini, oradan `api()` referansını alıp kendi `entity` kimliğini parametre olarak geçmektedir. Kendi gövdesi üzerindeki hız/kuvvet işlemleri için nesne yönelimli ve sezgisel bir arayüz bulunmamaktadır.
* **Kamera ve Varlık Arama:**
  ```cpp
  auto* camT = reg->get_component<transform_component>(find_by_name("MainCamera"));
  ```
  *Sorun:* İsimle arama her karede (veya değişken durumlarda) çağrıldığında ECS üzerinde string araması yapılması performans maliyeti doğurur ve ham pointer döner.

#### B. `camera_controller.cpp` Analizi
Dosya: `Application/Assets/Scripts/camera_controller.cpp`
* **Fare Girdisi:** `getContext().inputState.gameMouseDelta.y` doğrudan ham float olarak okunmaktadır.
* **Transform Manipülasyonu:** `camT.rotation.y = std::fmod(...)` şeklinde doğrudan transform bileşeninin vektörleri manuel radyan/derece hesaplamalarıyla ezilmektedir. Dönüşüm için kolaylaştırıcı fonksiyonlar (`rotate()`, `look_at()`, `forward()`, `right()`) yoktur.

---

### 2.3. Fizik Motoru (Jolt Physics) ve Çarpışma Olayları Durumu
Dosyalar: `Engine/Systems/physics_system.cpp`, `Engine/Systems/physics_bridge.cpp`

1. **Gövde Kullanıcı Verisi (`UserData`):**
   - `physics_bridge::create_physics_world` içerisinde:
     ```cpp
     body->SetUserData(static_cast<JPH::uint64>(ent));
     ```
     Bu durum **büyük bir avantajdır**. Jolt gövdeleri oluşturulurken ECS `entity` kimliği zaten Jolt'un `UserData` alanına kaydedilmektedir.
2. **Eksik Olan Kısım: `JPH::ContactListener`:**
   - `physics_system.cpp` içinde Jolt başlatılırken herhangi bir `ContactListener` atanmamıştır:
     ```cpp
     // mPhysicsSystem->SetContactListener(...) ÇAĞRILMAMIŞTIR!
     ```
   - Dolayısıyla Jolt dünyasında iki kutu veya küre çarpıştığında bu bilgi hiçbir yere iletilmemekte, betiklere haber verilememektedir.
3. **Çoklu İş Parçacığı (Multi-threading) Uyarısı:**
   - Jolt simülasyonu `JobSystemThreadPool` üzerinde çoklu iş parçacığıyla çalışmaktadır.
   - Jolt'un `ContactListener` fonksiyonları (`OnContactAdded`, `OnContactRemoved`) iş parçacıklarından tetiklenir. Doğrudan o fonksiyon içinde betik metotlarını çağırmak race condition (yarış durumu) yaratır.
   - Bu nedenle çarpışma olaylarının bir **çarpışma olay kuyruğuna (Collision Event Queue)** toplanması ve ana iş parçacığında güvenle dağıtılması gerekmektedir.
4. **Tetikleyici (Trigger) Eksikliği:**
   - `collider_component` içerisinde nesnenin fiziksel katı engel mi yoksa tetikleyici alan mı (`is_trigger = true/false`) olduğunu belirten bir alan bulunmamaktadır.

---

## 3. Özet Teşhis

| Konu | Mevcut Durum | İdeal Durum (Gameplay API) |
| :--- | :--- | :--- |
| **Motor Erişimi** | `getContext()` üzerinden sınırsız ham erişim | Yalnızca izin verilen sistemleri sunan `Gameplay / Scene API` |
| **Girdi (Input)** | `inputState.actions[(int)X]` ham dizi | `Input::is_action_pressed()`, `Input::get_axis()`, `Input::get_mouse_delta()` |
| **Fizik İşlemleri** | `physics.set_velocity(get_entity(), v)` | `this->rigidbody().set_velocity(v)` veya `physics::add_force(e, f)` |
| **Çarpışmalar** | Dinlenmiyor / Desteklenmiyor | `on_collision_enter(Collision&)`, `on_trigger_enter(Entity&)` |
| **Zaman Adımı** | Sadece değişken `on_update(dt)` | `on_update(dt)` + fizik için `on_fixed_update(fixed_dt)` |
| **Varlık ve Dönüşüm** | Ham ECS işaretçileri (`transform_component*`) | Güvenli `Entity` nesnesi, `transform().forward()`, `rotate_y()` |

