# 05 - Faz 1 Uygulama Raporu: ARIA İskelet & Animasyon İçe Aktarımı

## 1. Genel Bakış ve Hedefler

Bu aşamada (**Faz 1**), motorun mevcut hiçbir çalışan mekanizmasını (statik modeller, materyaller, sahneler, scriptler) bozmadan, **ARIA** varlık sistemine ve **Assimp** tabanlı yükleme hattına iskeletli modelleri (Mixamo vb. FBX/glTF), kemik hiyerarşisini, ters bağlanma matrislerini (Inverse Bind Pose Matrices) ve animasyon anahtar karelerini okuma kabiliyeti kazandırılmıştır.

---

## 2. Gerçekleştirilen Geliştirmeler

### 2.1. Ham Veri Yapıları (`asset_raw_structures.h`)
- **`raw_skinned_vertex`:** Konum, normal, UV ve tanjant verilerine ek olarak köşe başına maksimum 4 kemik etkisi (`glm::ivec4 bone_ids`, `glm::vec4 bone_weights`) tanımlandı.
- **`raw_skinned_mesh_data`:** Derili köşe ve indeks tamponlarını taşıyan, sınır küresi (local bounding sphere) hesaplamasını destekleyen yapı eklendi.
- **`bone_info` & `skeleton_node`:** Matris paleti indeksi, kemik adı, $M_{offset}$ (Inverse Bind Matrix) ve ebeveyn-çocuk düğüm ağacı modellendi.
- **`raw_skeleton_data`:** İskeletin kök düğümünü (`root_node`), düz kemik listesini (`bones`), $O(1)$ isim haritasını (`bone_mapping`) ve kök ters matrisini (`global_inverse_transform`) barındıran yapı eklendi.
- **Animasyon Yapıları:** `vector_keyframe`, `quat_keyframe`, `bone_animation_channel` ve `raw_animation_data` ile zaman damgalı konum, kuaterniyon rotasyon ve ölçek izleri modellendi.
- **`raw_skinned_model_data`:** Derili alt mesh koleksiyonu, bağlı iskelet ve gömülü animasyon kliplerini tek bir pakette toplayan ana CPU veri yapısı oluşturuldu.

### 2.2. Varlık Tanımları (`asset_structures.h`)
Motorun canlı kayıt defterinde saklanan varlık tipleri genişletildi:
- `skeleton_asset`: İskelet hiyerarşisi ve kemik verisi.
- `animation_clip_asset`: Anahtar kare ve kanal verisi.
- `skinned_mesh_asset`: GPU üzerindeki VAO, VBO, EBO kimlikleri ve indeks sayısı.
- `skinned_model_asset`: Alt mesh'ler, bağlı iskelet handle'ı ve animasyon handle'ları.

### 2.3. GPU Arka Uç ve VAO Konfigürasyonu (`render_backend.h`, `opengl_backend.h`, `opengl_backend.cpp`)
- `create_skinned_model` fonksiyonu eklendi.
- **Kritik OpenGL Nüansı:** Kemik ID'leri `location = 4` yuvasına **`glVertexAttribIPointer(4, 4, GL_INT, ...)`** fonksiyonu ile tamsayı olarak bağlandı. Ağırlıklar ise `location = 5` yuvasına `glVertexAttribPointer(5, 4, GL_FLOAT, ...)` ile bağlandı.

### 2.4. Assimp Tabanlı İçe Aktarım Hattı (`asset_loader.h`, `asset_loader.cpp`)
- **Assimp -> GLM Dönüştürücüleri:** `assimp_to_glm_mat4` (row-major -> column-major), `assimp_to_glm_vec3` ve `assimp_to_glm_quat` yardımcı fonksiyonları eklendi.
- **Kemik & Ağırlık Ayrıştırma (`process_assimp_skinned_mesh`):**
  - Mesh kemikleri taranarak iskelete kaydedildi.
  - Her bir köşe için ağırlıklar toplandı, en yüksek etkiye sahip 4 kemik seçildi ve toplamları kesinlikle $1.0$ olacak şekilde normalize edildi.
- **Hiyerarşi Ağacı İnşası (`build_skeleton_node`):** Assimp sahne ağacındaki düğümler taranarak iskelet ağacı oluşturuldu ve kemik indeksleriyle eşleştirildi.
- **Animasyon Ayrıştırma (`process_assimp_animation`):** Position, Rotation (Quat) ve Scaling anahtar kareleri kanallara dönüştürüldü.
- **API Fonksiyonları:**
  - `load_skinned_model_to_cpu(file_path)`
  - `load_skeleton_to_cpu(file_path)`
  - `load_animations_to_cpu(file_path)`

### 2.5. Kayıt Defteri & GPU Önbelleği (`asset_registry`, `gpu_resource_cache`, `aria`)
- `asset_registry` içerisine nesilsel (generational) `m_skeleton_slots`, `m_animation_slots` ve `m_skinned_model_slots` eklendi.
- `gpu_resource_cache` içerisine derili modellerin JIT (Just-In-Time) donanım tamponlarını yöneten ve gerektiğinde bellekten tahliye eden (eviction) mekanizma eklendi.
- `aria::system` arayüzüne `import_skinned_model`, `import_skeleton`, `import_animation`, `get_skinned_submesh_info` ve bellek temizleme fonksiyonları kazandırıldı.

---

## 3. Derleme & Doğrulama Sonuçları

- **Engine.lib:** 0 Hata, 0 Kritik Uyarı ile başarıyla derlendi.
- **Application.exe:** Motorun tüm alt sistemleri, oyun scriptleri ve render hattı ile sıfır gerileme (zero regression) garantisiyle başarıyla bağlandı (link edildi).
- **Geriye Dönük Uyumluluk:** Mevcut statik modeller (`raw_vertex`, `mesh_component`, `load_model_to_cpu`) %100 orijinal davranışlarını korumaktadır.

---

## 4. Bir Sonraki Adım (Faz 2)

- **Faz 2:** GPU Deri Yüzme (Skinning) Shader'ının (`skinned_default.vert`) hazırlanması, UBO (Uniform Buffer Object) üzerinden kemik matris paletinin bağlanması ve render hattına entegrasyonu.

