# 03 - Prosedürel Animasyon & Inverse Kinematics (IK) Temeli

## 1. Prosedürel Animasyon ve IK İhtiyacı

Geleneksel anahtar kare (keyframe) animasyonları statik ortamlarda harika görünür; ancak dinamik bir oyun dünyasında şu durumlarda yetersiz kalır:
- **Eğimli ve Merdivenli Zeminler:** Karakterin bir ayağı havada kalır veya zeminin içine batar (Foot Sliding / Foot Clipping).
- **Bakış ve Nişan Alma (Look-At / Aiming):** Karakterin kafası veya silahı kameranın baktığı hedefe doğru dinamik olarak dönmelidir.
- **Darbe ve Tepki (Hit Reaction / Ragdoll Blend):** Mermi veya kılıç darbesinde omurganın geriye esnemesi.
- **Fiziksel Nesneleri Tutma:** Karakterin elinin dinamik bir kapı koluna veya duvara uzanması.

Bu hedeflere ulaşmak için, iskelet sisteminin kemikleri **çalışma zamanında dinamik olarak sorgulanabilir, kısıtlanabilir ve modifiye edilebilir** bir mimariye sahip olması gerekir.

---

## 2. Kemik Erişim & Modifikasyon API'si (Scripting / Gameplay Seviyesi)

Oyun geliştiricisi (veya gameplay scripti) iskelet üzerindeki kemiklere isimleriyle veya indeksleriyle sezgisel şekilde erişebilmelidir:

```cpp
class skeleton_manipulator {
public:
    // --- KEMİK BİLGİSİ SORGULAMA ---
    [[nodiscard]] int find_bone_index(const std::string& bone_name) const;
    [[nodiscard]] bool has_bone(const std::string& bone_name) const;

    // --- YEREL POZ ERİŞİMİ (Animatör Sonrası, Hiyerarşi Öncesi) ---
    [[nodiscard]] bone_transform get_bone_local_transform(int bone_index) const;
    void set_bone_local_transform(int bone_index, const bone_transform& transform);
    void modify_bone_local_rotation(int bone_index, const glm::quat& delta_rotation);

    // --- DÜNYA / MODEL UZAYI POZ ERİŞİMİ (Hiyerarşi Çözüldükten Sonra) ---
    [[nodiscard]] glm::vec3 get_bone_model_position(int bone_index) const;
    [[nodiscard]] glm::vec3 get_bone_world_position(int bone_index, const glm::mat4& entity_world_matrix) const;
    [[nodiscard]] glm::mat4 get_bone_world_matrix(int bone_index, const glm::mat4& entity_world_matrix) const;
};
```

---

## 3. IK Algoritmaları Seçimi & Tasarımı

Farklı vücut bölgeleri için farklı matematiksel çözücüler (solvers) kullanılır:

| IK Algoritması | Kullanım Alanı | Matematiksel Yaklaşım | Avantajı |
| :--- | :--- | :--- | :--- |
| **Two-Bone IK** | Bacaklar (Kalça-Diz-Ayak bileği)<br>Kollar (Omuz-Dirsek-Bilek) | Analitik (Kosinüs Teoremi tabanlı üçgen çözümü) | **0 yineleme maliyeti (aşırı hızlı)**, deterministik, titreme (jitter) yapmaz. |
| **FABRIK** | Omurga (Spine), Boyun, Yılan/Kuyruk | Heuristic (İleri-Geri uzanma ve uzunluk koruma) | Çok eklemli uzun zincirlerde kararlı, eklem limitlerine uyumlu. |
| **Look-At / Aim IK** | Kafa (`Head`), Boyun (`Neck`), Göğüs (`Chest`) | Analitik Rotasyon (Quat LookAt + Açı Kısıtlaması) | Hedefe anlık ve yumuşak (damping) yönelim sağlar. |

---

## 4. İki Kemikli Analitik IK (Two-Bone IK) ve Zemin Adaptasyonu (Foot Placement)

Karakterin eğimli zeminde ayaklarının yere tam oturmasını sağlayan zemin IK boru hattı şu 4 adımdan oluşur:

```
[1. Zemin Raycast'i (Jolt Physics)]
             |
             v (Ayak tabanının basması gereken hedef yükseklik ve yüzey normali)
[2. Pelvis / Hips Dengeleme]
             |
             v (Karakterin bacağı gerilmesin diye kalçayı aşağı indir)
[3. Two-Bone IK Çözümü (Kalça -> Diz -> Ayak)]
             |
             v (Kosinüs teoremi ile diz açısını ve dizin baktığı yönü - Pole Vector hesapla)
[4. Ayak Tabanı Hizalama (Foot Ankle Rotation)]
             v (Ayak bileğini zemin normaline göre eğ)
```

### 4.1. Two-Bone IK Matematiksel Formülasyonu
- $A$: Başlangıç eklemi (Kalça / Omuz)
- $B$: Orta eklem (Diz / Dirsek)
- $C$: Uç eklem (Ayak bileği / El bileği)
- $T$: Ulaşılmak istenen hedef konum (Target)
- $l_1 = |B - A|$: Üst kemik uzunluğu
- $l_2 = |C - B|$: Alt kemik uzunluğu
- $d = |T - A|$: Kalçadan hedefe olan mesafe

Kosinüs Teoremi ile dizdeki iç açı $\beta$:
$$\cos(\beta) = \frac{l_1^2 + l_2^2 - d^2}{2 \cdot l_1 \cdot l_2}$$
Kalçadaki iç açı $\alpha$:
$$\cos(\alpha) = \frac{l_1^2 + d^2 - l_2^2}{2 \cdot l_1 \cdot d}$$

Eğer $d \ge l_1 + l_2$ ise bacak tamamen gerilir.
**Pole Vector (Kutup Vektörü):** Dizin sağa sola garip şekilde bükülmesini engellemek için, diz kapağının bakması gereken yön (genellikle karakterin ileri vektörü) referans alınarak çözüm düzlemi sabitlenir.

---

## 5. Katmanlı Animasyon ve Kemik Maskeleme (Bone Masking)

Mixamo'dan gelen klipler tek başına tüm vücudu hareket ettirir. Örneğin "Run" animasyonu kollar ve bacakları koştururken; karakter ateş etmek istediğinde üst vücutta "Rifle Aim" animasyonunun oynaması gerekir.

Bunu sağlamak için **Kemik Maskeleme (Bone Masking)** mimarisi kullanılır:
- **Alt Vücut Maskesi:** `mixamorig:Hips`, `mixamorig:LeftLeg*`, `mixamorig:RightLeg*` -> Koşma klibinden gelen yerel pozları kullanır.
- **Üst Vücut Maskesi:** `mixamorig:Spine*`, `mixamorig:LeftArm*`, `mixamorig:RightArm*`, `mixamorig:Head` -> Nişan alma klibinden gelen yerel pozları kullanır.
- **Blend İşlemi:** Yerel TRS pozları üzerinde Slerp (küresel kuaterniyon enterpolasyonu) ile katmanlar ağırlıklı olarak toplanır:
  $$Q_{final} = \text{Slerp}(Q_{base}, Q_{upper}, w_{mask})$$

---

## 6. Prosedürel İskelet Bileşeni Tasarımı (`ik_rig_component`)

```cpp
/**
 * @struct two_bone_ik_target
 * @brief İki kemikli zincir için hedef verileri.
 */
struct two_bone_ik_target {
    int root_bone_index{ -1 };    ///< Kalça veya omuz
    int mid_bone_index{ -1 };     ///< Diz veya dirsek
    int end_bone_index{ -1 };     ///< Ayak veya el

    glm::vec3 target_position{ 0.0f };
    glm::vec3 pole_vector{ 0.0f, 0.0f, 1.0f }; ///< Dizin bükülme yönü
    float weight{ 1.0f };         ///< 0.0 = Saf animasyon, 1.0 = Tam IK etkisi
};

/**
 * @struct look_at_ik_target
 * @brief Kafa ve boyun için bakış hedefi.
 */
struct look_at_ik_target {
    int bone_index{ -1 };
    glm::vec3 target_world_pos{ 0.0f };
    float max_angle_degrees{ 75.0f }; ///< İnsan anatomisi sınırları
    float weight{ 1.0f };
};

/**
 * @struct ik_rig_component
 * @brief Karakterin prosedürel IK ayarlarını tutan ECS bileşeni.
 */
struct ik_rig_component {
    bool enable_foot_ik{ true };
    float foot_raycast_distance{ 1.2f };
    float foot_offset_y{ 0.05f };

    two_bone_ik_target left_leg;
    two_bone_ik_target right_leg;
    look_at_ik_target  head_look_at;
};
```

