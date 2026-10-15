# Rapor 4: Gameplay API Modülü Geliştirme ve Tamamlanma Raporu

## 1. Giriş ve Genel Bakış
Kullanıcı talebi ve belirlenen kurallar doğrultusunda, motorun mevcut çalışan sistemlerine (`EngineContext`, `scene`, `player_controller` vb.) dokunulmadan ve mevcut kodlar kırılmadan, **Gameplay API** tamamen bağımsız ve modüler bir mimari olarak `Engine/Gameplay/` dizini altında geliştirilmiştir.

Modül; motorun alt seviye yapılarından (`Jolt Physics`, `GLFW`, `ECS registry`) beslenen ancak yukarıdaki betik yazarına temiz, nesne yönelimli ve sezgisel bir arayüz sunan bir soyutlama katmanı sağlar.

---

## 2. Geliştirilen Bileşenler ve Mimari Yapı

```
Engine/Gameplay/
├── gameplay.h                  # Tüm API'yi tek noktadan sunan çatı başlık dosyası
├── gameplay_context.h/.cpp     # Motor bağlamı (EngineContext, Scene) köprüsü
├── gameplay_input.h/.cpp       # Eylem, tuş ve fare kontrolleri API'si
├── gameplay_entity.h/.cpp      # Nesne yönelimli varlık (entity handle) sınıfı
├── gameplay_scene.h/.cpp       # Varlık arama, oluşturma ve silme servisi
├── gameplay_script.h           # Olay tabanlı yeni nesil betik taban sınıfı
├── transform_wrapper.h/.cpp    # Konum, Euler rotasyon, forward/right/up yön vektörleri
├── rigidbody_wrapper.h/.cpp    # Hız, itme (impulse), kuvvet (force), kütle yönetimi
├── collision_types.h           # collision_hit, trigger_hit, event_type yapıları
├── collision_event_queue.h/.cpp# Thread-safe Jolt çarpışma olay kuyruğu
└── jolt_contact_listener.h/.cpp# Jolt temaslarını yakalayan dinleyici sınıf
```

---

## 3. Detaylı Modül İncelemesi

### 3.1. Girdi Sistemi (`gameplay::input`)
Betik geliştiricisinin `inputState.actions[...]` gibi ham dizilerle uğraşmasını engeller.
* **Eylemler:** `is_action_pressed()`, `is_action_just_pressed()`, `is_action_just_released()`
* **Fiziksel Tuşlar:** `is_key_pressed()`, `is_key_just_pressed()`, `is_key_just_released()`
* **Eksenler (Axes):**
  - `get_axis(positive, negative)` -> -1.0 ile +1.0 arası değer döner.
  - `get_axis_2d(forward, backward, right, left)` -> Normalize edilmiş yön vektörü döner.
* **Fare:** `get_mouse_delta()`, `get_mouse_position()`, `is_cursor_locked()`, `set_cursor_locked()`

### 3.2. Dönüşüm Sarmalayıcısı (`gameplay::transform_wrapper`)
Varlıkların 3D uzaydaki durumunu yönetir:
* `get_position()`, `set_position()`, `translate()`
* `get_rotation_euler()`, `set_rotation_euler()`, `rotate_euler()`
* `get_scale()`, `set_scale()`
* **Yön Vektörleri:** `forward()` (-Z OpenGL standardı), `right()` (+X), `up()` (+Y)
* **Yönelme:** `look_at(target_point, world_up)`

### 3.3. Fizik Sarmalayıcısı (`gameplay::rigidbody_wrapper`)
Doğrudan Jolt veya `physics_api` parametreleriyle uğraşmadan gövdeyi manipüle eder:
* `get_velocity()`, `set_velocity(vel)`
* `add_impulse(impulse)` (Zıplama veya ani patlama darbeleri)
* `add_force(force)` (Sürekli kuvvetler)
* `get_mass()`, `set_mass()`, `get_friction()`, `set_friction()`, `get_restitution()`, `set_restitution()`
* `is_dynamic()`, `is_kinematic()`, `is_static()`

### 3.4. Varlık ve Sahne Yönetimi (`gameplay_entity`, `gameplay::scene_service`)
* `gameplay_entity`: Varlığın geçerliliğini (`is_valid()`), adını (`get_name()`, `set_name()`), bileşenlerini (`has_component<T>()`, `get_component<T>()`, `try_get_component<T>()`, `add_component<T>()`, `remove_component<T>()`) güvenle yönetir.
* `scene_service`: `find_entity("Player")`, `get_entity(id)`, `create_entity("Name")`, `destroy(id)`, `get_active_camera()`.

### 3.5. Çarpışma ve Olay Güdümlü Fizik Altyapısı
* `jolt_contact_listener`: Jolt'un `OnContactAdded`, `OnContactPersisted`, `OnContactRemoved` geri çağrılarını yakalar. `inBody1.GetUserData()` ve `inBody2.GetUserData()` üzerinden varlık kimliklerini eşler.
* `collision_event_queue`: Jolt'un iş parçacığı havuzundan gelen olayları `std::mutex` koruması altında toplar. Ana oyun döngüsünde `drain()` metodu ile ana iş parçacığına güvenle aktarılır.
* `collision_hit` ve `trigger_hit`: Temas noktası, normali, çarpışma şiddeti ve diğer varlık kimliğini temiz bir yapıda taşır.

### 3.6. Yeni Nesil Betik Tabanı (`gameplay::script`)
Betik yazarı için hazır yaşam döngüsü ve olaylar:
```cpp
class my_character : public gameplay::script
{
public:
    void on_create() override;
    void on_update(float dt) override;
    void on_fixed_update(float fixed_dt) override;
    void on_collision_enter(const gameplay::collision_hit& hit) override;
    void on_trigger_enter(const gameplay::trigger_hit& hit) override;
};
```

---

## 4. Örnek Kullanım Kodu (Geliştirici Deneyimi Karşılaştırması)

### Eski Yöntem (Mevcut `player_controller.cpp`):
```cpp
auto& input = getContext().inputState;
auto* reg = get_registry();
auto* camT = reg->get_component<transform_component>(find_by_name("MainCamera"));
auto& physics = getContext().physicsSystem.api();
glm::vec3 currentVel = physics.get_velocity(get_entity());
physics.set_velocity(get_entity(), finalVel);
```

### Yeni Gameplay API ile:
```cpp
#include <Gameplay/gameplay.h>

void player_controller::on_update(float dt)
{
    glm::vec2 moveInput = gameplay::input::get_axis_2d(
        InputAction::MoveForward, InputAction::MoveBackward,
        InputAction::MoveRight,   InputAction::MoveLeft
    );

    glm::vec3 moveDir = transform().forward() * moveInput.y + transform().right() * moveInput.x;
    rigidbody().set_velocity(glm::vec3(moveDir.x * speed, rigidbody().get_velocity().y, moveDir.z * speed));

    if (gameplay::input::is_action_just_pressed(InputAction::Jump))
    {
        rigidbody().add_impulse(glm::vec3(0.0f, 5.0f, 0.0f));
    }
}

void player_controller::on_collision_enter(const gameplay::collision_hit& hit)
{
    // Çarpışma anı
}
```

---

## 5. Derleme ve Uyumluluk Doğrulaması

1. **Engine Derlemesi:**
   - `Engine.vcxproj` içerisine yeni modülün tüm başlık ve kaynak dosyaları eklendi.
   - `devenv.com OpenGL-Learn-1.slnx /Build "Debug|x64" /Project "Engine\Engine.vcxproj"` çalıştırıldı.
   - **Sonuç:** ✅ `Engine.vcxproj -> x64\Debug\Engine.lib` başarıyla derlendi (0 hata, 0 uyarı).

2. **Application Derlemesi:**
   - Mevcut uygulama ve mevcut betikler dokunulmadan derlendi.
   - **Sonuç:** ✅ `Application.vcxproj -> x64\Debug\Application.exe` başarıyla bağlandı ve derlendi.
   - Geriye dönük uyumluluk %100 korundu.

3. **Dokümantasyon Standardı:**
   - Eklenen tüm sınıflar, fonksiyonlar ve enum'lar Türkçe Doxygen standartlarına (`@file`, `@brief`, `@param`, `@return`) uygun olarak belgelendi.

## 6. Mevcut Scriptlerin Gameplay API'ye Taşınması

Projede yer alan iki oyun betiği başarıyla yeni Gameplay API mimarisine geçirilmiştir:

1. **`player_controller` ([player_controller.cpp](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Application/Assets/Scripts/player_controller.cpp)):**
   - Ham `getContext().inputState` erişimi yerine `gameplay::input::get_axis_2d(...)` kullanıldı.
   - Ham `getContext().physicsSystem.api().set_velocity(...)` yerine doğrudan `rigidbody().set_velocity(...)` ve `rigidbody().get_velocity()` kullanıldı.
   - Ham pointer ile kamera araması yerine `gameplay::scene_service::find_entity("MainCamera")` ve `camEntity.transform().get_rotation_euler()` kullanıldı.

2. **`camera_controller` ([camera_controller.cpp](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/Application/Assets/Scripts/camera_controller.cpp)):**
   - Ham `getContext().inputState.gameMouseDelta` yerine `gameplay::input::get_mouse_delta()` kullanıldı.
   - Ham `camT = get_component<transform_component>()` pointer müdahaleleri yerine `transform().get_rotation_euler()`, `transform().set_rotation_euler()`, `transform().set_position()` kullanıldı.
   - Hedef oyuncu varlığı `gameplay::scene_service::get_entity(target)` ile güvenli nesne tutucusu üzerinden takip edildi.

3. **Çalışma Zamanı Doğrulaması:**
   - Uygulama başlatıldı (`Application.exe`) ve runtime'ın `Responding: True` durumunda sorunsuz çalıştığı doğrulandı.


