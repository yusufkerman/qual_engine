# 01 - ARIA Varlık Hattı & Assimp İskelet İçe Aktarımı

## 1. Giriş ve Mevcut Durum Analizi

Mevcut **ARIA** varlık sistemi (`aria::system`), 3D modelleri `asset_loader::load_model_to_cpu` üzerinden Assimp kütüphanesini kullanarak içe aktarmaktadır. Güncel akış şu şekildedir:

1. **`asset_loader::load_model_to_cpu(file_path)`**:
   - Assimp'in `ReadFile` fonksiyonunu çağırır (`aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace`).
   - `process_assimp_node` fonksiyonu ile Assimp sahne ağacını dolaşır.
   - `process_assimp_mesh` ile her bir `aiMesh` nesnesini motorun `raw_mesh_data` yapısına dönüştürür.
2. **Mevcut Köşe (Vertex) Yapısı (`raw_vertex`)**:
   ```cpp
   struct raw_vertex {
       glm::vec3 position{ 0.0f };  // loc 0
       glm::vec3 normal{ 0.0f };    // loc 1
       glm::vec2 tex_coord{ 0.0f }; // loc 2
       glm::vec3 tangent{ 0.0f };   // loc 3
   };
   ```
3. **Mevcut Sınırlılık:**
   - Model içindeki kemik bilgileri (`aiMesh::mBones`), kemik ağırlıkları (`aiVertexWeight`), ters bağlanma matrisleri (Inverse Bind Pose Matrices) ve iskelet hiyerarşisi (`aiNode` ağacı) tamamen göz ardı edilmektedir.
   - Model statik bir geometri olarak yüklenmekte; karakter veya yaratık modelleri t-pose veya default pozda donuk kalmaktadır.

---

## 2. Assimp ile İskelet ve Kemik Verilerinin Ayrıştırılması

Assimp, iskelet ve animasyon verilerini iki farklı yerde tutar:
1. **Deri/Mesh Ağırlıkları (`aiMesh::mBones`):** Hangi köşenin hangi kemiklerden ne kadar etkilendiği ve mesh uzayından kemik uzayına geçiş matrisi (`aiBone::mOffsetMatrix`).
2. **İskelet Hiyerarşisi (`aiScene::mRootNode`):** Kemiklerin ebeveyn-çocuk ilişkisi ve t-pose'daki yerel dönüşümleri (`aiNode::mTransformation`).

### 2.1. aiBone Yapısı ve İnverse Bind Pose Matrisi
Her bir `aiBone` nesnesi şunları içerir:
- `mName`: Kemiğin adı (Örn: `"mixamorig:RightArm"` veya `"mixamorig:Hips"`).
- `mOffsetMatrix`: **Inverse Bind Pose Matrix ($M_{offset}$)**.
  - Bu matris, köşeyi **Mesh/Model uzayından** kemiğin **kendi yerel referans uzayına** taşır.
  - Formül: $V_{bone} = M_{offset} \cdot V_{mesh}$
- `mWeights`: Köşe ağırlık dizisi (`aiVertexWeight`). Hangi köşe indeksinin (`mVertexId`) bu kemikten ne kadar (`mWeight`) etkilendiğini belirtir.

### 2.2. Köşe Başına Kemik Sınırı ve Normalizasyon (Max 4 Bones per Vertex)
Oyun motoru endüstrisinde (Unreal, Unity, Godot) standart olarak köşe başına **en fazla 4 kemik etkisi** (`ivec4 bone_ids`, `vec4 weights`) kullanılır. Bu, 32-bit GPU mimarilerinde bellek hizalaması (alignment) ve shader performansı için altın orandır. Mixamo modelleri genellikle köşe başına 1 ile 4 arasında kemik ağırlığı barındırır.

**Ağırlık Normalizasyonu Kuralı:**
Tek bir köşe üzerindeki tüm kemik ağırlıklarının toplamı **kesinlikle 1.0** olmalıdır:
$$\sum_{i=0}^{3} w_i = 1.0$$
Eğer bir köşeye 4'ten fazla kemik etki ediyorsa, en büyük ağırlığa sahip ilk 4 kemik seçilir ve toplamları 1.0 olacak şekilde yeniden normalize edilir.

---

## 3. Yeni Veri Yapıları ve ARIA Varlık Tasarımı

### 3.1. Genişletilmiş Köşe Yapısı (`raw_skinned_vertex` vs `raw_vertex`)

İki mimari seçenek mevcuttur:

| Kriter | Seçenek A: Tek Birleşik `raw_vertex` | Seçenek B: Ayrı `raw_skinned_vertex` (Önerilen) |
| :--- | :--- | :--- |
| **Açıklama** | Mevcut `raw_vertex` içine `glm::ivec4 bone_ids` ve `glm::vec4 weights` eklenir. | Statik modeller için `raw_vertex` (32 byte), iskeletli modeller için `raw_skinned_vertex` (64 byte) kullanılır. |
| **Bellek Tüketimi** | Statik çevre nesneleri (bina, taş, ağaç) gereksiz yere 32 byte ek vertex yükü taşır (%100 VRAM israfı). | Statik nesneler kompakt kalır, sadece iskeletli modeller ek veriyi taşır. |
| **Shader Karmaşıklığı** | Tek bir vertex layout, ancak shader'da `is_skinned` dallanması. | Ayrık VAO konfigürasyonu ve optimize edilmiş Skinned Shader. |
| **Geriye Dönük Uyumluluk** | Mevcut tüm objelerin stride ve pointer offsetlerini değiştirebilir. | Mevcut sistemleri **asla bozmaz**, sıfır regresyon riski. |

> [!IMPORTANT]
> **Öneri:** **Seçenek B** tercih edilmelidir. Mevcut `raw_vertex` ve `raw_mesh_data` aynen korunacak; ARIA sistemine `raw_skinned_vertex`, `raw_skinned_mesh_data` ve `skinned_mesh_asset` eklenecektir.

```cpp
namespace aria {

    constexpr int MAX_BONES_PER_VERTEX = 4;
    constexpr int MAX_BONES_PER_SKELETON = 128; // Mixamo modelleri genelde 50-70 kemiktir.

    /**
     * @struct raw_skinned_vertex
     * @brief İskeletli modeller için kemik indeks ve ağırlıklarını barındıran köşe yapısı.
     */
    struct raw_skinned_vertex {
        glm::vec3 position{ 0.0f };                           // loc 0 (12 bytes)
        glm::vec3 normal{ 0.0f };                             // loc 1 (12 bytes)
        glm::vec2 tex_coord{ 0.0f };                          // loc 2 (8 bytes)
        glm::vec3 tangent{ 0.0f };                            // loc 3 (12 bytes)
        glm::ivec4 bone_ids{ -1, -1, -1, -1 };                // loc 4 (16 bytes - IVEC4)
        glm::vec4 bone_weights{ 0.0f, 0.0f, 0.0f, 0.0f };     // loc 5 (16 bytes - VEC4)
    };

}
```

### 3.2. İskelet Varlığı (`skeleton_asset`)
Bir modelin içindeki kemik hiyerarşisi, mesh verisinden ayrı bir varlık veya modelin bir alt bileşeni olarak temsil edilmelidir. Böylece aynı iskeleti paylaşan farklı modeller (örneğin aynı karaktere giydirilen farklı zırh/kıyafet modelleri) tek bir iskeleti yeniden kullanabilir.

```cpp
namespace aria {

    /**
     * @struct bone_info
     * @brief Tek bir kemiğin temel tanıtıcı ve bağlama (bind pose) verileri.
     */
    struct bone_info {
        int id{ -1 };                         ///< Matris paletindeki benzersiz kemik indeksi (0..MAX_BONES-1).
        std::string name;                     ///< Kemiğin adı (Örn: "mixamorig:RightArm").
        glm::mat4 offset_matrix{ 1.0f };      ///< Inverse Bind Pose Matrix.
    };

    /**
     * @struct skeleton_node
     * @brief İskelet hiyerarşik ağacındaki tek bir düğüm (Assimp aiNode karşılığı).
     */
    struct skeleton_node {
        std::string name;
        int bone_index{ -1 };                 ///< Eğer bir kemik ise bone_info indeksi, değilse -1 (yardımcı düğüm).
        glm::mat4 local_transform{ 1.0f };    ///< Ebeveynine göre T-Pose yerel dönüşüm matrisi.
        std::vector<skeleton_node> children;  ///< Alt eklem / kemik çocukları.
    };

    /**
     * @struct skeleton_asset
     * @brief Bir modele ait tam iskelet tanımı.
     */
    struct skeleton_asset {
        asset_guid guid;
        std::string name;
        skeleton_node root_node;                              ///< Hiyerarşi ağacının kök düğümü.
        std::vector<bone_info> bones;                         ///< Düz kemik listesi (Hızlı indeks erişimi için).
        std::unordered_map<std::string, int> bone_mapping;    ///< İsimden kemik indeksine O(1) haritalama.
        glm::mat4 global_inverse_transform{ 1.0f };           ///< Kök düğümün ters matrisi.
    };

}
```

### 3.3. Animasyon Klibi Varlığı (`animation_clip_asset`)
Mixamo'dan indirilen animasyon FBX dosyaları veya model içine gömülü animasyonlar Assimp tarafından `aiScene::mAnimations` olarak sunulur.

```cpp
namespace aria {

    struct vector_keyframe {
        float time_stamp{ 0.0f };
        glm::vec3 value{ 0.0f };
    };

    struct quat_keyframe {
        float time_stamp{ 0.0f };
        glm::quat value{ 1.0f, 0.0f, 0.0f, 0.0f }; // Kuaterniyon (Slerp enterpolasyonu için)
    };

    /**
     * @struct bone_animation_channel
     * @brief Belirli bir kemiğin zaman içindeki konum, rotasyon ve ölçek izleri.
     */
    struct bone_animation_channel {
        std::string bone_name;
        std::vector<vector_keyframe> positions;
        std::vector<quat_keyframe>   rotations;
        std::vector<vector_keyframe> scalings;
    };

    /**
     * @struct animation_clip_asset
     * @brief Çalışma zamanında oynatılabilir tekil bir animasyon klibi (Koşma, Yürüme, Zıplama).
     */
    struct animation_clip_asset {
        asset_guid guid;
        std::string name;
        float duration{ 0.0f };               ///< Klibin toplam süresi (Tick cinsinden).
        float ticks_per_second{ 25.0f };       ///< Saniyedeki tick frekansı (Genelde 30 veya 24 FPS).
        std::vector<bone_animation_channel> channels;
    };

}
```

---

## 4. Mixamo FBX İçe Aktarımındaki Kritik Özel Durumlar

Mixamo FBX dosyaları oyun motorlarına entegre edilirken sıkça karşılaşılan ve çözülmesi gereken tipik problemler:

1. **Ölçekleme Tutarsızlığı (Unit Scaling - cm vs m):**
   - Mixamo ve 3ds Max/Maya dışa aktarımları genellikle 1 birim = 1 santimetre olarak çalışır. OpenGL dünyasında 1 birim = 1 metre kabul edildiğinde karakterler 100 kat büyük görünebilir.
   - Assimp bayrağı: `aiProcess_GlobalScale` veya kök dönüşüm matrisinin ölçek faktörünün kontrolü sağlanmalıdır.
2. **Kök Düğüm & Boş Düğümler (`Armature` & `mixamorig:Hips`):**
   - Blender ve Mixamo genellikle kemiklerin tepesine `Armature` adında bir dummy (yardımcı) sahne düğümü ekler. Kök hareket (Root Motion) veya karakter dönüşümü hesabı yapılırken gerçek kök kemik (`mixamorig:Hips`) ile dummy düğümün birbirine karıştırılmaması gerekir.
3. **Kuaterniyon Rotasyon Sıralaması & W Bileşeni:**
   - Assimp kuaterniyonları `(w, x, y, z)` formatında verirken glm::quat varsayılan kurucusunda `(w, x, y, z)` veya `(x, y, z, w)` sıralamasına dikkat edilmelidir:
     `glm::quat q(aiQuat.w, aiQuat.x, aiQuat.y, aiQuat.z);`
4. **T-Pose Model ile Animasyon FBX Ayrımı:**
   - Mixamo'dan iş akışı genellikle şöyledir:
     - 1 adet "Character Skin" (T-Pose karakter modeli + İskelet).
     - N adet "Without Skin" FBX animasyon klibi (Sadece kemik keyframe'leri, mesh içermez).
   - ARIA `load_model_to_cpu` fonksiyonu dosya içinde mesh bulamadığında bunu bir hata olarak görmemeli, bunun bir **salt animasyon klibi** (`animation_clip_asset`) olduğunu algılayıp sisteme o şekilde kaydetmelidir.

