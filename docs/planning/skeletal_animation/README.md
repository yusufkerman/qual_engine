# İskelet ve Animasyon Sistemi Mimari Planı (Skeletal Animation & IK System)

Bu dizin, **OpenGL-Learn-1** motoruna Mixamo ve benzeri standart 3D kaynaklardan iskelet (skeleton/rig), kemik hiyerarşisi (bone hierarchy), animasyon klipleri (animation clips), GPU skinning ve prosedürel animasyon / IK (Inverse Kinematics) desteği kazandırmak amacıyla hazırlanan ön mimari raporları ve yol haritasını içerir.

---

## 📑 Rapor Dizini

1. [01. ARIA Varlık Hattı & Assimp İskelet İçe Aktarımı](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/01_skeletal_data_pipeline_and_aria_import.md)
   - Mevcut ARIA yapısının (`raw_vertex`, `raw_mesh_data`, `model_asset`) analizi.
   - Assimp ile kemik (`aiBone`), kemik ağırlıkları (weights), inverse bind pose matrisleri ve iskelet hiyerarşisinin okunması.
   - Mixamo FBX formatındaki özel durumlar (ölçekleme, `mixamorig:` adlandırma standardı, kök düğüm).
   - Yeni ARIA varlık türleri: `skeleton_asset` ve `animation_clip_asset`.

2. [02. ECS Bileşen Mimarisi & Çalışma Zamanı Veri Modeli](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/02_ecs_component_architecture.md)
   - `skeleton_component`, `animator_component`, `skinned_mesh_component` ve `bone_attachment_component` tasarımları.
   - Çalışma zamanı pose hiyerarşisi: Yerel (Local) dönüşümler -> Global (Model) dönüşümler -> Son Deri Matris Paleti (Skinning Palette).
   - SERA serileştirme ve motorun mevcut ECS altyapısıyla uyum.

3. [03. Prosedürel Animasyon & Inverse Kinematics (IK) Temeli](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/03_procedural_animation_and_ik_foundation.md)
   - Kemiklere kod üzerinden erişim API'si (yerel/global matris modifikasyonu).
   - İki Kemikli Analitik IK (Two-Bone IK - Ayak basma / bacak & kol adaptasyonu).
   - FABRIK (Forward And Backward Reaching Inverse Kinematics - omurga, kuyruk, çok eklemli zincirler).
   - Bakış/Nişan Alma IK (Look-At / Aim IK) ve Jolt Physics zemin raycast entegrasyonu.
   - Katmanlı Animasyon (Layered Blending) ve Kemik Maskeleme (Bone Masking).

4. [04. GPU Deri Yüzme (Skinning) & Grafik Boru Hattı](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/04_gpu_skinning_and_rendering_pipeline.md)
   - GPU Skinning mimarisi (Linear Blend Skinning - LBS vs Dual Quaternion Skinning - DQS).
   - Vertex layout değişimi: `glVertexAttribIPointer` (kemik ID'leri) ve `glVertexAttribPointer` (ağırlıklar).
   - Matris paletini GPU'ya iletme stratejileri: SSBO (Shader Storage Buffer Object) vs UBO (Uniform Buffer Object).
   - GLSL Skinned Vertex Shader tasarımı.

5. [05. Faz 1 Uygulama Raporu: ARIA İskelet & Animasyon İçe Aktarımı](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/05_phase1_implementation_report.md)
   - Faz 1 kapsamında uygulanan veri yapıları, Assimp ayrıştırma hattı ve derleme doğrulama sonuçları.

6. [06. Faz 2 Uygulama Raporu: GPU Deri Yüzme & Shader Hattı](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/06_phase2_implementation_report.md)
   - Faz 2 kapsamında geliştirilen `skinned_default` vertex/fragment shader'ları, UBO matris paleti ve `MeshRenderer` entegrasyon sonuçları.

7. [07. Faz 3 Uygulama Raporu: ECS Bileşen Mimarisi & Animasyon Yürütme Motoru](file:///c:/Users/yusuf/source/repos/OpenGL-Learn-1/docs/planning/skeletal_animation/07_phase3_implementation_report.md)
   - Faz 3 kapsamında geliştirilen `skinned_mesh_component`, `skeleton_component`, `animator_component`, `bone_attachment_component`, `animation_system` ve `render_system` entegrasyon sonuçları.

---

## 🎯 Temel Hedefler & Tasarım İlkeleri

- **Geriye Dönük Uyumluluk:** Mevcut statik meshler (`raw_vertex`, `mesh_component`, `MeshRenderer`) bozulmayacak; statik ve dinamik/skinned çizim hatları temiz bir şekilde ayrılacaktır.
- **Mixamo Uyumlu (Plug & Play):** Mixamo'dan indirilen T-Pose veya animasyonlu FBX dosyaları ek bir manuel işlem gerektirmeden doğru yönelim (Y-up) ve doğru hiyerarşiyle doğrudan okunacaktır.
- **Performans:** İskelet hesaplamaları ve kemik matris dönüşümleri CPU tarafında önbellek dostu (Data-Oriented/DOD) düz dizilerde hesaplanacak, vertex deformasyonu tamamen GPU vertex shader üzerinde (GPU Skinning) icra edilecektir.
- **IK ve Prosedürel Hazırlık:** Kemik dönüşümleri, animasyon klibinden gelen pose ile kod/fizikten gelen IK dönüşümlerini temiz bir sıra (pipeline stage) dahilinde birleştirebilecek yapıda olacaktır.

