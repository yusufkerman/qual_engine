#version 330 core

/**
 * @file default_shader.frag
 * @brief Modern Cook-Torrance PBR (Metallic-Roughness) fragment shader.
 * Desteklenen özellikler: GGX Normal Dağılımı, Smith Geometrik Gölgeleme, Roughness-Aware Fresnel-Schlick,
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
uniform vec3 baseColor = vec3(1.0);
uniform float u_Metallic = 0.0;
uniform float u_Roughness = 0.7;
uniform float u_AO = 1.0;
uniform vec3 u_EmissiveColor = vec3(0.0);
uniform float u_EmissiveIntensity = 0.0;
uniform vec2 u_TextureScale = vec2(1.0);

// Küresel PBR Render Settings Kontrolleri
uniform float u_PbrAmbientIntensity = 0.22;
uniform float u_PbrIBLIntensity = 0.15;
uniform float u_PbrSpecularIntensity = 1.0;
uniform float u_PbrRoughnessMultiplier = 1.0;
uniform float u_PbrMetallicMultiplier = 1.0;
uniform float u_PbrExposure = 0.0;
uniform int u_PbrToneMapping = 2;

// Dolaylı Işık (Indirect Diffuse GI - Hemisphere & Hybrid SSGI)
uniform bool u_EnableGI = true;
uniform vec3 u_GISkyColor = vec3(0.52, 0.62, 0.78);
uniform vec3 u_GIGroundColor = vec3(0.28, 0.24, 0.19);

// Doku Girişleri ve Bayraklar
uniform bool useTexture;
uniform sampler2D diffuseTex;

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength = 1.0;

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
// SİS (DISTANCE + HEIGHT FOG + SUN IN-SCATTERING)
// ============================================================================
uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;
uniform int u_FogMode = 1;
uniform float u_FogHeightFalloff = 0.08;
uniform float u_FogBaseHeight = 0.0;
uniform float u_FogSunScattering = 0.35;
uniform bool u_FogLinkAmbient = true;
uniform bool u_VolumetricActive = false;
uniform float u_VolumetricMaxDistance = 80.0;

// PS1 Retro Horror Parametreleri
uniform bool u_PS1Mode = false;
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
    int   cast_shadows;
    vec2  _pad0;
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


// Spot ve Point Isik Golgeleri
uniform bool u_HasSpotShadow;
uniform sampler2D u_SpotShadowMap;
uniform mat4 u_SpotLightSpaceMatrix;

uniform bool u_HasPointShadow;
uniform samplerCube u_PointShadowMap;
uniform vec3 u_PointShadowLightPos;
uniform float u_PointShadowFarPlane;

float CalculateShadowSpot(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = max(0.003 * (1.0 - dot(N, L)), 0.0008);
    vec2 texelSize = 1.0 / textureSize(u_SpotShadowMap, 0);
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float depth = texture(u_SpotShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > depth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

const vec3 pointSampleOffsetDirections[20] = vec3[20](
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float CalculateShadowPoint(vec3 worldPos, vec3 lightPos, float farPlane, vec3 N)
{
    vec3 fragToLight = worldPos - lightPos;
    float currentDist = length(fragToLight);
    if (currentDist > farPlane) return 0.0;

    float d = max(abs(fragToLight.x), max(abs(fragToLight.y), abs(fragToLight.z)));
    float n = 0.1;
    float f = farPlane;
    float currentDepth = ((f + n) / (f - n) - (2.0 * f * n) / ((f - n) * d)) * 0.5 + 0.5;

    float bias = max(0.005 * (1.0 - dot(N, -normalize(fragToLight))), 0.001);
    float shadow = 0.0;
    float diskRadius = (1.0 + (currentDist / farPlane)) / 50.0;

    for (int i = 0; i < 20; ++i)
    {
        vec3 sampleDir = fragToLight + pointSampleOffsetDirections[i] * diskRadius;
        float closestDepth = texture(u_PointShadowMap, sampleDir).r;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    return shadow / 20.0;
}

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

// 3. Fresnel-Schlick Yaklaşımı (F) ve Pürüzlülük Duyarlı Fresnel (IBL için)
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 4. Lagarde Specular Occlusion (Sebastien Lagarde & Charles de Rousiers 2014)
// Ortam örtmesinden (AO), yüzey pürüzlülüğünden ve bakış açısından fiziksel aynasal örtme türetir.
float ComputeSpecularOcclusion(float NdotV, float ao, float roughness)
{
    return clamp(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

// ============================================================================
// ANA SHADER DÖNGÜSÜ (MAIN)
// ============================================================================
void main()
{
    vec2 tiledUV = vUV * u_TextureScale;

    // 1. Albedo ve Cutout Alfa Testi (baseColor çarpanı ile tam uyumlu)
    vec4 texSample = useTexture ? texture(diffuseTex, tiledUV) : vec4(1.0);
    vec4 texColor = vec4(texSample.rgb * baseColor, texSample.a);
    if (u_RenderMode == 1)
    {
        if (texColor.a < u_AlphaThreshold)
        {
            discard;
        }
    }

    // Standart PBR modunda tüm ışıklandırma fiziksel doğrusal (Linear HDR) uzayda hesaplanır;
    // Ton eşleme ve sRGB dönüşümü en son post-process aşamasında uygulanır.
    vec3 albedo = u_PS1Mode ? texColor.rgb : pow(max(texColor.rgb, vec3(0.0)), vec3(2.2));

    // 2. Normal Haritalama
    vec3 N = normalize(vNormal);
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, tiledUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }

    // 3. PBR Yüzey Özellikleri (Metallic, Roughness, AO)
    // glTF PBR standardında: Blue (B) = Metallic, Green (G) = Roughness, Red (R) = AO.
    // Ayrıca eski Specular iş akışından dönüştürülmüş hatalı/simsiyah (G==B < 0.12, R > 0.5)
    // dokuların tüm sahneyi 0.04 ıslak aynaya çevirmesini otomatik algılayıp engeller.
    float metallic = u_Metallic;
    if (u_UseMetallicMap)
    {
        vec4 mSample = texture(metallicTex, tiledUV);
        float mVal = mSample.b;
        if (mVal < 0.12 && abs(mSample.g - mSample.b) < 0.02 && mSample.r > 0.5)
        {
            mVal = 0.0;
        }
        metallic *= mVal;
    }
    metallic = clamp(metallic * u_PbrMetallicMultiplier, 0.0, 1.0);

    float roughness = u_Roughness;
    if (u_UseRoughnessMap)
    {
        vec4 rSample = texture(roughnessTex, tiledUV);
        float rVal = rSample.g;
        if (rVal < 0.12 && abs(rSample.g - rSample.b) < 0.02 && rSample.r > 0.5)
        {
            rVal = clamp(0.85 - rVal * 2.5, 0.50, 0.90);
        }
        roughness *= rVal;
    }
    roughness = clamp(roughness * u_PbrRoughnessMultiplier, 0.05, 1.0);

    float ao = u_AO;
    if (u_UseAOMap)
    {
        ao *= texture(aoTex, tiledUV).r;
    }
    ao = clamp(ao, 0.0, 1.0);

    // 4. Görüş Vektörü ve Temel Yansıtıcılık (F0)
    vec3 V = normalize(viewPos - vWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    // Dielektrik yüzeyler için 0.04 baz yansıtıcılık, metaller için albedo rengi
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

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
        vec3 lightLinearColor = u_PS1Mode ? l.color : pow(max(l.color, vec3(0.0)), vec3(2.2));
        // Direkt ışık şiddeti: HDR boru hattında doğal dinamik aralık için 2.4x fiziksel ölçek
        float directScale = u_PS1Mode ? 1.0 : 2.4;
        vec3 radiance = lightLinearColor * (l.intensity * directScale) * attenuation;

        // Cook-Torrance BRDF Hesabı
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * NdotV * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = (numerator / denominator) * u_PbrSpecularIntensity;

        // Enerji Korunumu (kS = Fresnel, kD = 1.0 - kS)
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);

        // Gölge Hesabı
        float shadow = 0.0;
        if (l.type == 0) // DIRECTIONAL
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
        else if (l.type == 1) // POINT
        {
            if (l.cast_shadows != 0 && u_HasPointShadow)
            {
                shadow = CalculateShadowPoint(vWorldPos, l.position, l.range, N);
            }
        }
        else if (l.type == 2) // SPOT
        {
            if (l.cast_shadows != 0 && u_HasSpotShadow)
            {
                vec4 fragPosSpot = u_SpotLightSpaceMatrix * vec4(vWorldPos, 1.0);
                shadow = CalculateShadowSpot(fragPosSpot, N, L);
            }
        }

        // Doğrudan yayılı (diffuse) bileşeni /PI ile aşırı sönümlemeden PSX kadar güçlü ve tepkisel tut
        vec3 diffuseTerm = kD * albedo;
        Lo += (1.0 - shadow) * (diffuseTerm + specular) * radiance * NdotL;
    }

    // 6. Dolaylı Işık (Hemisphere Irradiance GI) ve Specular IBL + Lagarde Specular Occlusion
    vec3 R = reflect(-V, N);

    vec3 skyColorLin = u_PS1Mode ? u_GISkyColor : pow(max(u_GISkyColor, vec3(0.0)), vec3(2.2));
    vec3 groundColorLin = u_PS1Mode ? u_GIGroundColor : pow(max(u_GIGroundColor, vec3(0.0)), vec3(2.2));

    vec3 envReflection;
    if (u_UseSkyboxReflection)
    {
        float lod = roughness * 7.0;
        envReflection = textureLod(u_SkyboxTex, R, lod).rgb;
        if (!u_PS1Mode)
        {
            envReflection = pow(max(envReflection, vec3(0.0)), vec3(2.2));
        }

        // Skybox en üst mip katmanından düşük frekanslı gökyüzü ışımasını (SH/Hemisphere yaklaşımı) harmanla
        vec3 skyIrrSample = textureLod(u_SkyboxTex, N, 6.5).rgb;
        if (!u_PS1Mode)
        {
            skyIrrSample = pow(max(skyIrrSample, vec3(0.0)), vec3(2.2));
        }
        skyColorLin = mix(skyColorLin, skyIrrSample, 0.35);
    }
    else
    {
        float horizon = clamp(R.y * 0.5 + 0.5, 0.0, 1.0);
        envReflection = mix(groundColorLin, skyColorLin, horizon);
    }

    // Birincil yönlü ışıktan (Güneş) zemine çarpıp yukarı seken analitik sıcak zemin sekmesi (Ground Bounce)
    vec3 primarySunDir = vec3(0.0, -1.0, 0.0);
    vec3 primarySunRadiance = vec3(0.0);
    for (int i = 0; i < lightCount && i < 64; ++i)
    {
        if (lights[i].type == 0)
        {
            primarySunDir = normalize(lights[i].direction);
            vec3 sCol = u_PS1Mode ? lights[i].color : pow(max(lights[i].color, vec3(0.0)), vec3(2.2));
            primarySunRadiance = sCol * lights[i].intensity;
            break;
        }
    }

    vec3 F_ambient = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD_amb = (vec3(1.0) - F_ambient) * (1.0 - metallic);

    // Yarım-küre (Hemisphere) Irradiance:
    // Yukarı bakan yüzeyler gökyüzü ışımasını (skyColorLin), aşağı bakan kemer/tavan yüzeyleri
    // ise güneşli zeminden seken sıcak dolaylı ışığı (groundBounce) alır.
    float hemiWeight = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sunGroundBounce = groundColorLin * (vec3(1.0) + primarySunRadiance * 0.45);
    vec3 hemiIrradiance = u_EnableGI ? mix(sunGroundBounce, skyColorLin, hemiWeight) : vec3(0.5);
    vec3 diffuseAmbient = u_PbrAmbientIntensity * hemiIrradiance * albedo * ao;

    // Aynasal çevresel yansıma (Specular IBL) + Lagarde Specular Occlusion + Horizon Occlusion
    float smoothness = 1.0 - roughness;
    float specAO = ComputeSpecularOcclusion(NdotV, ao, roughness);
    vec3 geomN = normalize(vNormal);
    float horizonOcclusion = clamp(1.0 + dot(R, geomN), 0.0, 1.0);
    horizonOcclusion *= horizonOcclusion;

    float iblWeight = u_PbrIBLIntensity * mix(smoothness * smoothness * 0.65 + 0.10, 1.0, metallic);
    vec3 specularAmbient = envReflection * F_ambient * iblWeight * specAO * horizonOcclusion;

    vec3 indirectLight = kD_amb * diffuseAmbient + specularAmbient;

    vec3 emissive = u_EmissiveColor * u_EmissiveIntensity;
    if (u_UseEmissiveMap)
    {
        vec3 emSample = texture(emissiveTex, tiledUV).rgb;
        emissive *= (u_PS1Mode ? emSample : pow(max(emSample, vec3(0.0)), vec3(2.2)));
    }

    vec3 directLight = Lo + emissive;
    vec3 color = indirectLight + directLight;

    // AO (SSAO/GTAO) yalnızca dolaylı (indirect) ışığa uygulansın, direkt güneş ışığını karartmasın:
    // Bu amaçla toplam ışık içindeki dolaylı ışık oranını (indirectWeight) hesaplayıp alfa kanalına kodluyoruz.
    float lumIndirect = dot(indirectLight, vec3(0.2126, 0.7152, 0.0722));
    float lumDirect   = dot(directLight, vec3(0.2126, 0.7152, 0.0722));
    float indirectWeight = clamp(lumIndirect / max(lumIndirect + lumDirect, 0.0001), 0.0, 1.0);

    // 7. Atmosferik Sis (Mesafe Sisi + Yükseklik Sisi + Güneş Saçılımı & Volumetric Uyumluluğu)
    if (u_FogEnabled)
    {
        vec3 camToFrag = vWorldPos - viewPos;
        float distance = length(camToFrag);
        vec3 viewDir = (distance > 0.0001) ? (camToFrag / distance) : vec3(0.0, 0.0, -1.0);

        // Volumetric God Rays aktifken yakın mesafede sis ve god ray çift sayılıp patlamasın
        float volRangeScale = u_VolumetricActive
            ? mix(0.45, 1.0, smoothstep(0.0, max(u_VolumetricMaxDistance, 1.0), distance))
            : 1.0;

        float effDensity = u_FogDensity * volRangeScale;
        float opticalDepth = 0.0;

        if (u_FogMode == 1) // Exponential Height Fog (Analitik Yükseklik İntegrali)
        {
            float falloff = max(u_FogHeightFalloff, 0.001);
            float camH = max(viewPos.y - u_FogBaseHeight, 0.0);
            float fragH = max(vWorldPos.y - u_FogBaseHeight, 0.0);
            float densityAtCam = effDensity * exp(-falloff * camH);
            float deltaH = fragH - camH;
            float heightIntegral = (abs(deltaH) > 0.001)
                ? ((1.0 - exp(-falloff * deltaH)) / (falloff * deltaH))
                : 1.0;
            opticalDepth = densityAtCam * distance * clamp(heightIntegral, 0.0, 10.0);
        }
        else // Exponential Distance Fog
        {
            opticalDepth = effDensity * distance;
        }

        float fogFactor = clamp(exp(-opticalDepth), 0.0, 1.0);

        vec3 baseFogSrgb = (u_FogLinkAmbient && u_EnableGI)
            ? mix(u_FogColor, u_GISkyColor, 0.65)
            : u_FogColor;
        vec3 fogBaseColor = u_PS1Mode ? baseFogSrgb : pow(max(baseFogSrgb, vec3(0.0)), vec3(2.2));

        // Güneş yönüne bakarken sisin güneş rengine kayması (Mie / Henyey-Greenstein Sun In-Scattering)
        float cosSun = max(dot(viewDir, -primarySunDir), 0.0);
        float sunPhase = pow(cosSun, 6.0) * 0.65 + pow(cosSun, 2.0) * 0.25;
        float sunAntiDouble = u_VolumetricActive
            ? smoothstep(u_VolumetricMaxDistance * 0.35, max(u_VolumetricMaxDistance, 1.0), distance)
            : 1.0;
        vec3 sunInScatter = primarySunRadiance * sunPhase * u_FogSunScattering * sunAntiDouble;

        vec3 finalFogColor = fogBaseColor + sunInScatter;
        color = mix(finalFogColor, color, fogFactor);
    }

    // 8. PS1 Retro Modu aktifse inline kontrast ve Bayer 4x4 dithering uygula;
    // Standart PBR modunda ise doğrusal HDR rengi doğrudan post_process_pass'e aktar!
    if (u_PS1Mode)
    {
        color = max(color, vec3(0.0));
        color = (color * (1.0 + color * 0.18)) / (1.0 + color * 0.45);
        color = clamp(color, 0.0, 1.0);

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
    }

    float finalAlpha = (u_RenderMode == 2) ? texColor.a : indirectWeight;
    FragColor = vec4(color, finalAlpha);
}
