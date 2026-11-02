#version 330 core

/**
 * @file default_shader.frag
 * @brief Modern Cook-Torrance PBR (Metallic-Roughness) fragment shader.
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

// Gökyüzü Çevre Yansıması (Skybox IBL Reflection)
uniform bool u_UseSkyboxReflection;
uniform samplerCube u_SkyboxTex;

// Çizim Modu (0: Opaque, 1: Cutout, 2: Transparent)
uniform int u_RenderMode;
uniform float u_AlphaThreshold;

// ============================================================================
// SIS (EXPONENTIAL FOG)
// ============================================================================
uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;

// PS1 Retro Horror Parametreleri
uniform float u_PS1ColorDepth;
uniform bool u_PS1Dithering;
uniform float u_PS1DitherStrength;

const float ps1_bayer4x4[16] = float[16](
     0.0 / 16.0,  8.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
    12.0 / 16.0,  4.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0,
     3.0 / 16.0, 11.0 / 16.0,  1.0 / 16.0,  9.0 / 16.0,
    15.0 / 16.0,  7.0 / 16.0, 13.0 / 16.0,  5.0 / 16.0
);


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
// Cascaded Shadow Maps (CSM)
uniform bool u_EnableCSM;
uniform sampler2DArray u_ShadowMapArray;
uniform int u_CascadeCount;
uniform float u_CascadePlaneDistances[3];
uniform mat4 u_CascadeLightSpaceMatrices[3];

float CalculateShadowCSM(vec3 worldPos, vec3 N, vec3 L, float viewDepth)
{
    int layer = 2;
    for (int i = 0; i < 3; ++i)
    {
        if (viewDepth < u_CascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }

    vec4 fragPosLightSpace = u_CascadeLightSpaceMatrices[layer] * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Gölge sınırları dışındaki pikseller gölgede bırakılmaz
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = max(0.002 * (1.0 - dot(N, L)), 0.0005) * (1.0 + float(layer) * 1.5);
    if (layer == 0) bias *= 0.6;

    vec2 texelSize = 1.0 / vec2(textureSize(u_ShadowMapArray, 0).xy);
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float depth = texture(u_ShadowMapArray, vec3(projCoords.xy + vec2(x, y) * texelSize, float(layer))).r;
            shadow += (projCoords.z - bias > depth) ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

float CalculateShadow(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = max(0.002 * (1.0 - dot(N, L)), 0.0005);
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
            if (u_EnableCSM)
            {
                float viewDepth = length(vWorldPos - viewPos);
                shadow = CalculateShadowCSM(vWorldPos, N, L, viewDepth);
            }
            else
            {
                shadow = CalculateShadow(vFragPosLightSpace, N, L);
            }
        }

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // 6. Ortam Işığı (Ambient Lighting) ve Çevresel Yansıma (PBR IBL)
    vec3 R = reflect(-V, N);

    vec3 envReflection;
    if (u_UseSkyboxReflection)
    {
        // Roughness değerine göre cubemap LOD seviyesinden yansıma örnekleme
        float lod = roughness * 7.0;
        envReflection = textureLod(u_SkyboxTex, R, lod).rgb;
    }
    else
    {
        // Skybox kapalıysa pürüzsüz gökyüzü/zemin gradyanı yansıması (Y ekseni bazlı)
        vec3 skyGrad = vec3(0.50, 0.65, 0.90);
        vec3 groundGrad = vec3(0.12, 0.10, 0.08);
        float horizon = clamp(R.y * 0.5 + 0.5, 0.0, 1.0);
        envReflection = mix(groundGrad, skyGrad, horizon);
    }

    // Fresnel-Schlick ortam yansıması (Pürüzlülük sönümlemesi ile)
    vec3 F_ambient = FresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kS_amb = F_ambient;
    vec3 kD_amb = (vec3(1.0) - kS_amb) * (1.0 - metallic);

    vec3 diffuseAmbient = vec3(0.04) * albedo;
    // Metaller için pürüzlülüğe bağlı çevresel speküler yansıma (Asla kapkara kalmaz)
    vec3 specularAmbient = envReflection * F_ambient * (1.0 - roughness * 0.4);

    vec3 ambient = (kD_amb * diffuseAmbient + specularAmbient) * ao;

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

    // 9. PS1 Retro Dithering & Color Quantization (Posterization)
    if (u_PS1ColorDepth > 1.0)
    {
        if (u_PS1Dithering)
        {
            int bx = int(mod(gl_FragCoord.x, 4.0));
            int by = int(mod(gl_FragCoord.y, 4.0));
            float dither = ps1_bayer4x4[by * 4 + bx] - 0.5;
            color += (dither * u_PS1DitherStrength) / u_PS1ColorDepth;
        }
        color = clamp(color, 0.0, 1.0);
        color = floor(color * u_PS1ColorDepth) / (u_PS1ColorDepth - 1.0);
    }

    float finalAlpha = (u_RenderMode == 2) ? texColor.a : 1.0;
    FragColor = vec4(color, finalAlpha);
}