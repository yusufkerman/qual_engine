#version 330 core

/**
 * @file skinned_default.frag
 * @brief İskelet animasyonlu (GPU skinning) modeller için modern Cook-Torrance PBR fragment shader.
 * Desteklenen özellikler: GGX Normal Dağılımı, Smith Geometrik Gölgeleme, Fresnel-Schlick,
 * Albedo/Normal/Metallic/Roughness/AO/Emissive dokuları, çoklu ışık tipleri, gölge ve exponential sis.
 */

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in mat3 vTBN;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

// ============================================================================
// PBR MATERYAL PARAMETRELERİ (UNIFORMS)
// ============================================================================
uniform vec3 baseColor;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;
uniform vec3 u_EmissiveColor;
uniform float u_EmissiveIntensity;
uniform vec2 u_TextureScale;

// Doku Girişleri ve Bayraklar
uniform bool useTexture;
uniform sampler2D diffuseTex;

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength;

uniform bool u_UseMetallicMap;
uniform sampler2D metallicTex;

uniform bool u_UseRoughnessMap;
uniform sampler2D roughnessTex;

uniform bool u_UseAOMap;
uniform sampler2D aoTex;

uniform bool u_UseEmissiveMap;
uniform sampler2D emissiveTex;

// Kamera ve Gölge
uniform vec3 viewPos;
uniform sampler2D shadowMap;

// Çizim Modu (0: Opaque, 1: Cutout, 2: Transparent)
uniform int u_RenderMode;
uniform float u_AlphaThreshold;

// ============================================================================
// SIS (EXPONENTIAL FOG)
// ============================================================================
uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;

// ============================================================================
// IŞIK TAMPONU (UBO std140 - 64 IŞIK)
// ============================================================================
struct Light
{
    int   type;           // 0: DIRECTIONAL, 1: POINT, 2: SPOT
    vec3  position;

    float range;
    vec3  direction;

    float intensity;
    vec3  color;

    float inner_cone;
    float outer_cone;
    vec2  _shaderPad0;
};

layout (std140) uniform LightBuffer
{
    int   lightCount;
    int   _pad0;
    int   _pad1;
    int   _pad2;
    Light lights[64];
};

const float PI = 3.14159265359;

// ============================================================================
// GÖLGE HARİTASI (PCF 3x3)
// ============================================================================
float CalculateShadow(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.0015 * (1.0 - dot(N, L)), 0.0003);
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float depth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > depth) ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

// ============================================================================
// PBR COOK-TORRANCE BRDF FONKSİYONLARI
// ============================================================================

// 1. Trowbridge-Reitz GGX Normal Dağılım Fonksiyonu (NDF - D)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

// 2. Schlick-GGX Geometrik Gölgeleme / Maskeleme Fonksiyonu (G)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. Fresnel-Schlick Yaklaşımı (F)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ============================================================================
// ANA SHADER DÖNGÜSÜ (MAIN)
// ============================================================================
void main()
{
    vec2 tiledUV = vUV * u_TextureScale;

    // 1. Albedo ve Cutout Alfa Testi
    vec4 texColor = useTexture ? texture(diffuseTex, tiledUV) : vec4(baseColor, 1.0);
    if (u_RenderMode == 1)
    {
        if (texColor.a < u_AlphaThreshold)
        {
            discard;
        }
    }

    // sRGB renk uzayından doğrusal (Linear) renk uzayına dönüşüm
    vec3 albedo = pow(texColor.rgb, vec3(2.2));

    // 2. Normal Haritalama
    vec3 N = normalize(vNormal);
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, tiledUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }

    // 3. PBR Yüzey Özellikleri (Metallic, Roughness, AO)
    float metallic = u_Metallic;
    if (u_UseMetallicMap)
    {
        metallic *= texture(metallicTex, tiledUV).r;
    }
    metallic = clamp(metallic, 0.0, 1.0);

    float roughness = u_Roughness;
    if (u_UseRoughnessMap)
    {
        roughness *= texture(roughnessTex, tiledUV).r;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    float ao = u_AO;
    if (u_UseAOMap)
    {
        ao *= texture(aoTex, tiledUV).r;
    }

    // 4. Görüş Vektörü ve Temel Yansıtıcılık (F0)
    vec3 V = normalize(viewPos - vWorldPos);

    // Dielektrik yüzeyler için 0.04 baz yansıtıcılık, metaller için albedo rengi
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 5. Doğrudan Aydınlatma Entegrasyonu (Direct Lighting)
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < lightCount && i < 64; ++i)
    {
        Light l = lights[i];
        vec3 L;
        float attenuation = 1.0;

        if (l.type == 0) // DIRECTIONAL
        {
            L = normalize(-l.direction);
        }
        else if (l.type == 1) // POINT
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);
            attenuation = 1.0 / (1.0 + (dist / l.range) * (dist / l.range));
        }
        else // SPOT
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);

            vec3 lightDir = normalize(l.direction);
            float theta = dot(-lightDir, L);
            float inner = cos(radians(l.inner_cone));
            float outer = cos(radians(l.outer_cone));
            float spot = smoothstep(outer, inner, theta);

            float distanceAtt = 1.0 / (1.0 + (dist * dist) / (l.range * l.range));
            attenuation = distanceAtt * spot;
        }

        vec3 H = normalize(V + L);
        vec3 radiance = l.color * l.intensity * attenuation;

        // Cook-Torrance BRDF Hesabı
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;

        // Enerji Korunumu (kS = Fresnel, kD = 1.0 - kS)
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        // Yalnızca Directional ışıktan gölge hesapla
        float shadow = 0.0;
        if (l.type == 0)
        {
            shadow = CalculateShadow(vFragPosLightSpace, N, L);
        }

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // 6. Ortam Işığı (Ambient Lighting) ve Kendiliğinden Işıma (Emissive)
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 emissive = u_EmissiveColor * u_EmissiveIntensity;
    if (u_UseEmissiveMap)
    {
        emissive *= texture(emissiveTex, tiledUV).rgb;
    }

    vec3 color = ambient + Lo + emissive;

    // 7. Exponential Sis (Fog)
    if (u_FogEnabled)
    {
        float distance = length(viewPos - vWorldPos);
        float fogFactor = exp(-u_FogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        color = mix(u_FogColor, color, fogFactor);
    }

    // 8. HDR Ton Eşleme (Reinhard Tone Mapping) ve Gama Düzeltmesi (Gamma Correction)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    float finalAlpha = (u_RenderMode == 2) ? texColor.a : 1.0;
    FragColor = vec4(color, finalAlpha);
}
