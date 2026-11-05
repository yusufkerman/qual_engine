#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
smooth in vec2 vUV;
noperspective in vec2 vAffineUV;
in vec4 vFragPosLightSpace;

// ============================================================================
// MATERYAL PARAMETRELERİ
// ============================================================================
uniform vec3 baseColor;
uniform vec2 u_TextureScale;
uniform bool useTexture;
uniform sampler2D diffuseTex;

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength;

uniform vec3 u_EmissiveColor;
uniform float u_EmissiveIntensity;
uniform bool u_UseEmissiveMap;
uniform sampler2D emissiveTex;

uniform vec3 viewPos;
uniform sampler2D shadowMap;

// Çizim Modu (0: Opaque, 1: Cutout, 2: Transparent)
uniform int u_RenderMode;
uniform float u_AlphaThreshold;

// ============================================================================
// SİS (EXPONENTIAL HORROR FOG)
// ============================================================================
uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;

// ============================================================================
// PS1 RETRO HORROR EFEKTLERİ
// ============================================================================
uniform bool u_PS1AffineMapping;      // Doğrusal afin doku yüzmesi (noperspective)
uniform float u_PS1ColorDepth;        // 32.0 (15-bit RGB555), 16.0 (12-bit)
uniform bool u_PS1Dithering;          // Bayer 4x4 sıralı dither
uniform float u_PS1DitherStrength;    // Dither yoğunluğu

// Kaskatlı Gölge Haritalama (CSM)
uniform bool u_EnableCSM;
uniform sampler2DArray u_ShadowMapArray;
uniform int u_CascadeCount;
uniform float u_CascadePlaneDistances[3];
uniform mat4 u_CascadeLightSpaceMatrices[3];

// 4x4 Bayer Matrisi
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

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = max(0.003 * (1.0 - dot(N, L)), 0.0005) * (1.0 + float(layer) * 1.5);
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

    float bias = max(0.003 * (1.0 - dot(N, L)), 0.0005);
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

void main()
{
    // 1. Doku Koordinatları: Tercihe göre Donanım PS1 Afin Haritalama veya Standart
    vec2 chosenUV = u_PS1AffineMapping ? vAffineUV : vUV;
    vec2 tiledUV = chosenUV * u_TextureScale;

    // 2. Albedo / Difüz Doku
    vec4 texColor = vec4(1.0);
    if (useTexture)
    {
        texColor = texture(diffuseTex, tiledUV);
    }
    vec3 albedo = texColor.rgb * baseColor;

    // 3. Cutout Kırpma (Alpha Test)
    if (u_RenderMode == 1)
    {
        float alphaCutoff = (u_AlphaThreshold > 0.0) ? u_AlphaThreshold : 0.5;
        if (texColor.a < alphaCutoff)
            discard;
    }

    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vWorldPos);

    // 4. PS1 Ortam ve Işıklandırma (Retro Stylized Shading)
    // Ortam ışığı: Asla kapkara kalmaması için albedo ile harmanlanmış temel ışık
    vec3 ambient = vec3(0.25) * albedo;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < lightCount && i < 64; ++i)
    {
        Light l = lights[i];
        vec3 L;
        float att = 1.0;

        if (l.type == 0) // DIRECTIONAL
        {
            L = normalize(-l.direction);
        }
        else if (l.type == 1) // POINT
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);
            float distTerm = dist / max(l.range, 0.001);
            att = 1.0 / (1.0 + distTerm * distTerm);
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

            float distTerm = dist / max(l.range, 0.001);
            att = (1.0 / (1.0 + distTerm * distTerm)) * spot;
        }

        float NdotL = max(dot(N, L), 0.0);
        vec3 lightRadiance = l.color * l.intensity * att;

        // Gölge hesaplama
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

        // Difüz + Hafif retro speküler parıltı
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 16.0);
        lighting += (1.0 - shadow) * (albedo * NdotL + vec3(spec * 0.25)) * lightRadiance;
    }

    // Emissive (Öz-aydınlatma)
    vec3 emissive = u_EmissiveColor * u_EmissiveIntensity;
    if (u_UseEmissiveMap)
    {
        emissive *= texture(emissiveTex, tiledUV).rgb;
    }

    vec3 finalColor = ambient + lighting + emissive;

    // 5. Exponential Horror Fog (Silent Hill / Resident Evil Atmosferi)
    if (u_FogEnabled)
    {
        float dist = length(viewPos - vWorldPos);
        float fogFactor = exp(-u_FogDensity * dist);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        finalColor = mix(u_FogColor, finalColor, fogFactor);
    }

    // 6. PS1 Sıralı Dithering & Renk Kuantizasyonu (15-bit RGB555 / 12-bit)
    if (u_PS1ColorDepth > 1.0)
    {
        if (u_PS1Dithering)
        {
            int bx = int(mod(gl_FragCoord.x, 4.0));
            int by = int(mod(gl_FragCoord.y, 4.0));
            float dither = ps1_bayer4x4[by * 4 + bx] - 0.5;
            finalColor += (dither * u_PS1DitherStrength) / u_PS1ColorDepth;
        }
        finalColor = clamp(finalColor, 0.0, 1.0);
        finalColor = floor(finalColor * u_PS1ColorDepth) / (u_PS1ColorDepth - 1.0);
    }

    float finalAlpha = (u_RenderMode == 2) ? texColor.a : 1.0;
    FragColor = vec4(finalColor, finalAlpha);
}
