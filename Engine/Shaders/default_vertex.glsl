#version 330 core

// ============================================================================
// GİRDİ (IN) DEĞİŞKENLERİ - VBO (Vertex Buffer Object) Veri Yapısı
// ============================================================================
layout(location = 0) in vec3 aPos;     // Köşe (Vertex) yerel konumu
layout(location = 1) in vec3 aNormal;  // Köşe yüzey normali
layout(location = 2) in vec2 aUV;      // Doku (Texture) koordinatı
layout(location = 3) in vec3 aTangent; // Yüzey teğet (Tangent) vektörü

// ============================================================================
// MATRİS UNIFORM DEĞİŞKENLERİ
// ============================================================================
uniform mat4 model;       // Yerel uzaydan -> Dünya uzayına dönüşüm matrisi
uniform mat4 view;        // Dünya uzayından -> Kamera (Görünüm) uzayına dönüşüm matrisi
uniform mat4 projection;  // Kamera uzayından -> Ekran (İzdüşüm) uzayına dönüşüm matrisi

// ============================================================================
// ÇIKTI (OUT) DEĞİŞKENLERİ - Fragment Shader'a aktarılacak veriler
// ============================================================================
out vec3 vWorldPos; // Dünya uzayındaki köşe konumu
out vec3 vNormal;   // Dünya uzayındaki normal vektörü
out vec2 vUV;       // Doku koordinatları
out mat3 vTBN;      // Teğet uzayı (Tangent Space) dönüşüm matrisi

// ============================================================================
// ANA ÇİZİM FONKSİYONU
// ============================================================================
void main()
{
    // 1. Köşenin dünya uzayındaki konumunun hesaplanması
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // 2. Normal Matrisi: Eşitsiz ölçeklendirme (Non-uniform scaling) kaynaklı bozulmaları önler
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    // 3. TBN (Tangent, Bitangent, Normal) Matrisinin Hesaplanması
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    
    // Gram-Schmidt Dikleştirme (Orthogonalization): 
    // Teğet vektörünün, normal vektörüne kusursuz bir şekilde dik olmasını garanti altına alır.
    T = normalize(T - dot(T, N) * N); 
    
    vec3 B = cross(N, T); // Bitangent vektörünün vektörel çarpım ile elde edilmesi

    vTBN = mat3(T, B, N);
    vNormal = N;

    // 4. Doku koordinatlarının doğrudan aktarılması
    vUV = aUV;

    // 5. Köşenin son ekran (Clip Space) koordinatının belirlenmesi
    gl_Position = projection * view * worldPos;
}