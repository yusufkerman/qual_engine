#version 330 core

/**
 * @file volumetric.frag
 * @brief Hacimsel ışıklandırma (Volumetric Lighting / Crepuscular God Rays) ışın yürütme shader'ı.
 *
 * Yönlü ışık (Directional Light) ve Kaskatlı Gölge Haritaları (CSM) ile entegre çalışarak
 * kamera ışını boyunca ışık saçılımını (In-scattering) ve sönümlemeyi (Extinction) hesaplar.
 * PS1 retro modu aktif olduğunda 4x4 Bayer matrisi ile dithering ve renk derinliği kuantizasyonu uygular.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

// Derinlik ve Gölge Sampler'ları
uniform sampler2D u_SceneDepth;
uniform sampler2D u_ShadowMap;
uniform sampler2DArray u_ShadowMapArray;

// Kamera ve Projeksiyon
uniform vec3 u_CameraPos;
uniform mat4 u_InvViewProj;

// Yönlü Işık (Directional Light)
uniform vec3 u_LightDir;        // Işığın yayılma yönü
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

// Hacimsel Parametreler
uniform float u_VolumetricIntensity;
uniform int u_Steps;
uniform float u_ScatteringG;    // Henyey-Greenstein asimetri katsayısı (-0.9 .. 0.95)
uniform float u_Density;        // Sis / partikül yoğunluğu (sigma_t)
uniform float u_MaxDistance;    // Azami ışın yürüme mesafesi

// Atmosferik Yükseklik Sisi (Height Fog) Entegrasyonu
uniform bool u_FogEnabled;
uniform int u_FogMode;
uniform float u_FogHeightFalloff;
uniform float u_FogBaseHeight;

// Gölge Ayarları
uniform bool u_EnableShadows;
uniform bool u_EnableCSM;
uniform int u_CascadeCount;
uniform float u_CascadePlaneDistances[3];
uniform mat4 u_CascadeLightSpaceMatrices[3];
uniform mat4 u_LightSpaceMatrix;

// PS1 Retro Horror Parametreleri
uniform bool u_PS1Mode;
uniform bool u_PS1Dithering;
uniform float u_PS1DitherStrength;
uniform float u_PS1ColorDepth;

#define PI 3.14159265358979323846

/**
 * @brief 4x4 Bayer matrisinden [0, 1) aralığında dither değeri döner.
 */
float Bayer4x4(vec2 coord)
{
    int x = int(mod(coord.x, 4.0));
    int y = int(mod(coord.y, 4.0));
    int index = x + y * 4;

    const float bayer[16] = float[16](
         0.0 / 16.0,  8.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
        12.0 / 16.0,  4.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0,
         3.0 / 16.0, 11.0 / 16.0,  1.0 / 16.0,  9.0 / 16.0,
        15.0 / 16.0,  7.0 / 16.0, 13.0 / 16.0,  5.0 / 16.0
    );
    return bayer[index];
}

/**
 * @brief Henyey-Greenstein faz fonksiyonu.
 * @param cosTheta Bakış yönü ile ışığa giden vektör arasındaki kosinüs değeri.
 * @param g Asimetri faktörü (g > 0 ileriye saçılma, g < 0 geriye saçılma).
 */
float HenyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * PI * pow(max(denom, 0.0001), 1.5));
}

void main()
{
    // 1. Sahne derinlik tamponundan piksel derinliğini oku
    float rawDepth = texture(u_SceneDepth, v_TexCoords).r;

    // Derinliği Normalized Device Coordinates [-1, 1] uzayına ve ardından Dünya uzayına dönüştür
    vec4 ndc = vec4(v_TexCoords * 2.0 - 1.0, rawDepth * 2.0 - 1.0, 1.0);
    vec4 worldTarget = u_InvViewProj * ndc;
    worldTarget /= worldTarget.w;

    vec3 rayVector = worldTarget.xyz - u_CameraPos;
    float totalDistance = length(rayVector);
    if (totalDistance < 0.0001)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 rayDir = rayVector / totalDistance;

    // Eğer arka plan (skybox veya sonsuz) ise mesafeyi u_MaxDistance ile sınırla
    float marchDistance = min(totalDistance, u_MaxDistance);
    if (rawDepth >= 0.99999)
    {
        marchDistance = u_MaxDistance;
    }

    if (marchDistance <= 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    int steps = clamp(u_Steps, 4, 128);
    float stepSize = marchDistance / float(steps);

    // Işın başlangıç ofsetini piksel bazında dither ile karıştırarak şeritlenmeyi (banding) önle
    float dither = Bayer4x4(gl_FragCoord.xy);

    // Henyey-Greenstein fazı (Işığa doğru giden vektör ile kamera ışını açısı)
    vec3 lightDirNorm = normalize(u_LightDir);
    vec3 toLight = -lightDirNorm;
    float cosTheta = dot(rayDir, toLight);
    float g = clamp(u_ScatteringG, -0.95, 0.95);
    float phase = HenyeyGreenstein(cosTheta, g);

    vec3 accumulatedLight = vec3(0.0);
    float transmittance = 1.0;
    float sigmaT = max(u_Density, 0.0001); // Soğurma ve saçılma toplam yoğunluğu

    for (int i = 0; i < steps; ++i)
    {
        float t = (float(i) + dither) * stepSize;
        if (t > marchDistance)
        {
            break;
        }

        vec3 samplePos = u_CameraPos + rayDir * t;

        // Gölge testi: Örneklem noktası gölgede mi?
        float shadow = 0.0;
        if (u_EnableShadows)
        {
            if (u_EnableCSM && u_CascadeCount > 0)
            {
                float viewDepth = length(samplePos - u_CameraPos);
                int layer = u_CascadeCount - 1;
                for (int c = 0; c < 3; ++c)
                {
                    if (c < u_CascadeCount && viewDepth < u_CascadePlaneDistances[c])
                    {
                        layer = c;
                        break;
                    }
                }

                vec4 lightSpacePos = u_CascadeLightSpaceMatrices[layer] * vec4(samplePos, 1.0);
                vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                projCoords = projCoords * 0.5 + 0.5;

                if (projCoords.z >= 0.0 && projCoords.z <= 1.0 &&
                    projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
                    projCoords.y >= 0.0 && projCoords.y <= 1.0)
                {
                    float closestDepth = texture(u_ShadowMapArray, vec3(projCoords.xy, float(layer))).r;
                    shadow = (projCoords.z - 0.001 > closestDepth) ? 1.0 : 0.0;
                }
            }
            else
            {
                vec4 lightSpacePos = u_LightSpaceMatrix * vec4(samplePos, 1.0);
                vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
                projCoords = projCoords * 0.5 + 0.5;

                if (projCoords.z >= 0.0 && projCoords.z <= 1.0 &&
                    projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
                    projCoords.y >= 0.0 && projCoords.y <= 1.0)
                {
                    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
                    shadow = (projCoords.z - 0.001 > closestDepth) ? 1.0 : 0.0;
                }
            }
        }

        float visibility = 1.0 - shadow;

        // Yükseklik sisi (Height Fog) aktifse örneklem yüksekliğine göre lokal yoğunluğu ölçekle
        float localSigmaT = sigmaT;
        if (u_FogEnabled && u_FogMode == 1)
        {
            float heightAboveBase = max(samplePos.y - u_FogBaseHeight, 0.0);
            localSigmaT *= exp(-max(u_FogHeightFalloff, 0.0) * heightAboveBase);
        }

        // Saçılma ve sönümleme (Beer-Lambert & In-scattering)
        if (visibility > 0.001)
        {
            float stepExtinction = exp(-localSigmaT * stepSize);
            float stepScattering = localSigmaT * stepSize;
            vec3 inScattered = u_LightColor * u_LightIntensity * visibility * phase * stepScattering;
            accumulatedLight += inScattered * transmittance;
            transmittance *= stepExtinction;
        }
        else
        {
            transmittance *= exp(-localSigmaT * stepSize);
        }

        if (transmittance < 0.01)
        {
            break;
        }
    }

    vec3 color = accumulatedLight * u_VolumetricIntensity;

    // PS1 Retro Horror Modu: Dithering ve Renk Derinliği Kuantizasyonu
    if (u_PS1Mode)
    {
        float depthLevels = max(u_PS1ColorDepth, 4.0);
        if (u_PS1Dithering)
        {
            float ditherVal = (Bayer4x4(gl_FragCoord.xy) - 0.5) * (u_PS1DitherStrength / depthLevels);
            color += vec3(ditherVal);
        }
        color = floor(color * depthLevels + 0.5) / depthLevels;
        color = max(color, vec3(0.0));
    }

    FragColor = vec4(color, 1.0);
}

