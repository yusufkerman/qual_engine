# 04 - GPU Deri Yüzme (Skinning) & Grafik Boru Hattı

## 1. GPU Skinning vs CPU Skinning

3D karakter deformasyonu için iki temel yaklaşım bulunur:

1. **CPU Skinning (Eski / Yetersiz):**
   - Her karede 10.000 - 50.000 köşe CPU üzerinde matrislerle çarpılır ve her karede `glBufferSubData` ile GPU'ya geri yüklenir.
   - **Sonuç:** PCIe veriyolu tıkanıklığı, yüksek CPU yükü, çoklu karakterlerde ciddi FPS düşüşü.
2. **GPU Skinning (Modern / Standart):**
   - Karakterin köşe verileri GPU VRAM'inde **sabit** kalır.
   - Her karede CPU sadece kemik dönüşüm matrislerini (128 matris = ~8 KB) GPU'ya iletir.
   - Köşe çarpımları GPU Vertex Shader üzerinde paralel binlerce çekirdekte anında hesaplanır.
   - **Sonuç:** Sıfır PCIe darboğazı, 60+ FPS kararlı performans.

> [!IMPORTANT]
> **OpenGL-Learn-1** motoru için **GPU Skinning (Linear Blend Skinning - LBS)** mimarisi uygulanacaktır.

---

## 2. Linear Blend Skinning (LBS) Matematiksel Modeli

Bir köşe, etkisindeki 4 kemiğin ağırlıklı ortalamasıyla deforme edilir:

$$V_{skinned} = \sum_{i=0}^{3} w_i \cdot \left( M_{skinning}^{B_i} \cdot V_{bind} \right)$$

Burada:
- $V_{bind}$: Model dosyasından yüklenen orijinal köşe pozisyonu (`vec4(position, 1.0)`).
- $B_i$: Köşenin etkilendiği $i$. kemiğin ID'si (`bone_ids[i]`).
- $w_i$: $i$. kemiğin ağırlık katsayısı (`bone_weights[i]`).
- $M_{skinning}^{B_i}$: $B_i$ kemiğinin nihai deri matrisi ($M_{global}^{B_i} \cdot M_{offset}^{B_i}$).

Normal ve tanjant vektörleri de aynı şekilde yönlendirilir (Ölçekleme bozulmalarını önlemek için 3x3 matris kullanılır):
$$N_{skinned} = \sum_{i=0}^{3} w_i \cdot \left( \text{mat3}(M_{skinning}^{B_i}) \cdot N_{bind} \right)$$

---

## 3. Matris Paletinin GPU'ya Aktarımı (UBO vs SSBO)

GPU'ya her kare 128 adet `glm::mat4` göndermek için iki modern teknik mevcuttur:

| Yöntem | Boyut Limiti | Donanım Uyumluluğu | Avantajı / Dezavantajı |
| :--- | :--- | :--- | :--- |
| **UBO (Uniform Buffer Object)** | Genellikle 16 KB - 64 KB (128 matris = 8 KB) | OpenGL 3.1+ (Tüm cihazlar) | **Son derece hızlı önbellek erişimi**, evrensel uyumluluk. 128 kemik Mixamo için fazlasıyla yeterlidir (~55 kemik). |
| **SSBO (Shader Storage Buffer Object)** | 128 MB+ (Sınırsıza yakın) | OpenGL 4.3+ | Binlerce kemik, kalabalık (crowd) instancing için ideal; ancak eski donanımlarda kısıtlı olabilir. |

### Önerilen Çözüm: UBO (Uniform Buffer Object)
Mixamo modelleri ortalama 50-65 kemik içerdiğinden, 128 kemik kapasiteli bir UBO (8.192 byte) en kararlı, en hızlı ve geriye dönük en uyumlu çözümdür.

```glsl
// GLSL Vertex Shader UBO Tanımı
layout (std140, binding = 2) uniform BoneMatricesBlock {
    mat4 u_Bones[128];
};
```

---

## 4. OpenGL Vertex Nitelikleri (Attributes Layout) ve Kritik Hata Önlemi

Skinned köşe yapısının GPU'ya bağlanmasında OpenGL geliştiricilerinin en sık yaptığı kritik bir hata bulunur: **Tamsayı (integer) ID'leri yanlış fonksiyonla bağlamak.**

```cpp
// -------------------------------------------------------------
// DOĞRU BAĞLAMA (glVertexAttribIPointer - "I" HARFİNE DİKKAT!)
// -------------------------------------------------------------

// Konum (Location 0)
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, position));

// Normal (Location 1)
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, normal));

// UV (Location 2)
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, tex_coord));

// Tanjant (Location 3)
glEnableVertexAttribArray(3);
glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, tangent));

// KEMİK ID'LERİ (Location 4) - KRİTİK: glVertexAttribIPointer kullanılmalıdır!
// Normal glVertexAttribPointer kullanılırsa tamsayılar ondalıklı sayıya (float) dönüştürülür ve shader'da kemik indeksleri bozulur!
glEnableVertexAttribArray(4);
glVertexAttribIPointer(4, 4, GL_INT, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, bone_ids));

// KEMİK AĞIRLIKLARI (Location 5)
glEnableVertexAttribArray(5);
glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, bone_weights));
```

---

## 5. Skinned Vertex Shader Örneği (`skinned_default.vert`)

```glsl
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4 aWeights;

const int MAX_BONES = 128;

layout (std140) uniform CameraBlock {
    mat4 u_View;
    mat4 u_Projection;
};

uniform mat4 u_Model;

layout (std140) uniform BoneMatricesBlock {
    mat4 u_Bones[MAX_BONES];
};

out vec3 v_FragPos;
out vec2 v_TexCoords;
out vec3 v_Normal;
out mat3 v_TBN;

void main()
{
    // Kemik ağırlıkları toplamı kontrolü (Fallback emniyeti)
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
    
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    vec3 totalTangent = vec3(0.0);

    if (totalWeight > 0.0001)
    {
        for (int i = 0; i < 4; i++)
        {
            int boneID = aBoneIDs[i];
            float weight = aWeights[i];

            if (boneID >= 0 && boneID < MAX_BONES && weight > 0.0)
            {
                mat4 boneMat = u_Bones[boneID];
                totalPosition += (boneMat * vec4(aPos, 1.0)) * weight;
                
                mat3 normalMat = mat3(boneMat);
                totalNormal += (normalMat * aNormal) * weight;
                totalTangent += (normalMat * aTangent) * weight;
            }
        }
    }
    else
    {
        // Eğer köşe hiçbir kemiğe bağlı değilse orijinal pozisyonu koru
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
        totalTangent = aTangent;
    }

    vec4 worldPos = u_Model * totalPosition;
    v_FragPos = worldPos.xyz;
    v_TexCoords = aTexCoords;

    // Normal ve TBN matris dönüşümleri
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    vec3 N = normalize(normalMatrix * totalNormal);
    vec3 T = normalize(normalMatrix * totalTangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    v_TBN = mat3(T, B, N);
    v_Normal = N;

    gl_Position = u_Projection * u_View * worldPos;
}
```

---

## 6. Render Boru Hattı (`MeshRenderer` & `render_item`) Entegrasyonu

Mevcut `render_item` yapısına kemik matris paletine ait UBO kimliği veya matris işaretçisi eklenir:

```cpp
struct render_item {
    // ... mevcut alanlar (model, dokular, materyal özellikleri) ...

    // İskelet / Deri Yüzme Bilgileri
    bool is_skinned{ false };
    unsigned int bone_ubo_id{ 0 }; ///< Kemik matrislerini barındıran GPU UBO nesnesi.
};
```

`MeshRenderer::Draw` çağrıldığında:
1. Eğer `item.is_skinned == true` ise, `item.bone_ubo_id` ilgili uniform binding noktasına (`glBindBufferBase(GL_UNIFORM_BUFFER, 2, item.bone_ubo_id)`) bağlanır.
2. Çizim her zamanki gibi `glDrawElements` ile tek seferde gerçekleştirilir.
3. Çizim bitince binding çözülür.

Bu mimari sayesinde mevcut statik modeller en ufak bir ek maliyet veya değişiklik olmadan çalışmaya devam ederken; iskeletli modeller tam GPU hızlandırmasıyla yüksek kare hızlarında çizilir.

