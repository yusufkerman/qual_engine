#version 330 core

// ============================================================================
// GİRDİ (IN) VE ÇIKTI (OUT) DEĞİŞKENLERİ
// ============================================================================
in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in mat3 vTBN;

out vec4 FragColor;

// ============================================================================
// MATERYAL UNIFORM DEĞİŞKENLERİ
// ============================================================================
uniform vec3 baseColor;
uniform bool useTexture;
uniform sampler2D diffuseTex;
uniform vec2 u_TextureScale; // Doku koordinatları için ölçeklendirme (Tiling) çarpanı

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength;

// ============================================================================
// IŞIK VERİ YAPISI VE UNIFORM TAMPONU (std140)
// ============================================================================
struct Light
{
    int type;         // Işık Türü (0: Yönlü/Directional, 1: Noktasal/Point, 2: Spot)
    vec3 position;    // std140 kuralları gereği GLSL vec3 otomatik olarak 16-byte sınırına yerleşir
    float range;      // Işığın etki yarıçapı

    vec3 direction;   // İleri yön vektörü
    float intensity;  // Işık şiddeti (Parlaklık çarpanı)

    vec3 color;       // Işık rengi (RGB)
    float inner_cone; // Spot ışık iç koni açısı

    float outer_cone; // Spot ışık dış koni açısı
    // Not: C++ tarafındaki 12-byte padding (3 x float) ile yapı 80 byte'a tamamlanmaktadır.
};

layout (std140, binding = 0) uniform LightBuffer
{
    int   lightCount;
    int   _pad0;
    int   _pad1;
    int   _pad2;
    Light lights[64];
};

// ============================================================================
// ANA RENDER FONKSİYONU
// ============================================================================
void main()
{
    // UV koordinatlarının doku ölçeği (Tiling) ile yapılandırılması
    vec2 tiledUV = vUV * u_TextureScale;

    vec3 N = normalize(vNormal);

    // Normal Haritası (Normal Map) ve Teğet Uzayı (Tangent Space) hesaplaması
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, tiledUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }

    // Temel yüzey rengi (Albedo) çıkarımı
    vec3 albedo = useTexture ? texture(diffuseTex, tiledUV).rgb : baseColor;

    vec3 result = vec3(0.0);
    float ambient = 0.15; // Sabit ortam ışığı (Ambient)

    result += albedo * ambient;

    // Sahnedeki tüm ışık kaynakları için aydınlatma iterasyonu
    for (int i = 0; i < lightCount && i < 64; i++)
    {
        Light l = lights[i];

        vec3 L;
        float attenuation = 1.0;

        if (l.type == 0) // YÖNLÜ IŞIK (DIRECTIONAL)
        {
            L = normalize(-l.direction);
        }
        else if (l.type == 1) // NOKTASAL IŞIK (POINT)
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);

            // Mesafe bazlı zayıflama (Attenuation - Ters Kare Yasasına Yakınsama)
            attenuation = 1.0 / (1.0 + (dist / l.range) * (dist / l.range));
        }
        else // SPOT IŞIK
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = toLight / dist;

            vec3 lightDir = normalize(l.direction);
            float theta = dot(-lightDir, L);

            float inner = cos(radians(l.inner_cone));
            float outer = cos(radians(l.outer_cone));

            // Koni açıları arasında pürüzsüz geçiş (Smoothstep yumuşatması)
            float spot = smoothstep(outer, inner, theta);

            // Mesafe ve koni açısı tabanlı birleşik zayıflama
            float distanceAtt = 1.0 / (1.0 + (dist * dist) / (l.range * l.range));
            attenuation *= distanceAtt * spot; 
        }

        // Dağınık (Diffuse) ışık hesaplaması
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = albedo * l.color * l.intensity * diff * attenuation;
        
        result += diffuse;
    }

    FragColor = vec4(result, 1.0);
}