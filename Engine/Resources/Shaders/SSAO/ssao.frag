#version 330 core

/**
 * @file ssao.frag
 * @brief Ekran Uzayı Ortam Örtmesi (Screen Space Ambient Occlusion) hesaplama shader'ı.
 *
 * Derinlik tamponundan bakış uzayı (view-space) pozisyonunu ve türev normallerini rekonstrükte eder.
 * Yarım küre örneklemesi ile köşe ve girintilerdeki ortam gölgelenmesini (AO) hesaplar.
 * PS1 retro modu aktif olduğunda 4x4 Bayer matrisi ile dithering ve basamaklama uygular.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_SceneDepth;
uniform sampler2D u_NoiseTexture;

uniform vec3 u_Samples[32];
uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform vec2 u_ScreenSize;

uniform float u_Radius;
uniform float u_Bias;
uniform int u_SampleCount;
uniform float u_Intensity;

// PS1 Retro Parametreleri
uniform bool u_PS1Mode;
uniform bool u_PS1Dithering;
uniform float u_PS1DitherStrength;

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
 * @brief Derinlik dokusundan bakış uzayı (view-space) pozisyonunu hesaplar.
 */
vec3 getViewPos(vec2 uv)
{
    float depth = texture(u_SceneDepth, uv).r;
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPosH = u_InvProjection * clip;
    float w = (abs(viewPosH.w) > 0.00001) ? viewPosH.w : (viewPosH.w < 0.0 ? -0.00001 : 0.00001);
    return viewPosH.xyz / w;
}

void main()
{
    float rawDepth = texture(u_SceneDepth, v_TexCoords).r;

    // Gökyüzü / arka plan için (derinlik azami sınırda ise) gölgeleme yapma
    if (rawDepth >= 1.0 - 0.00005)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 viewPos = getViewPos(v_TexCoords);

    // Ekran uzayı türevleri ile geometrik yüzey normalini türet
    vec3 dPdx = dFdx(viewPos);
    vec3 dPdy = dFdy(viewPos);
    vec3 normal = cross(dPdx, dPdy);
    float normalLen = length(normal);
    if (normalLen < 0.0001)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }
    normal /= normalLen;

    // Normalin kameraya dönük olmasını garantiye al
    if (dot(normal, -normalize(viewPos)) < 0.0)
    {
        normal = -normal;
    }

    // 4x4 gürültü dokusu ile teğet taban (TBN) matrisi kur
    vec2 noiseScale = u_ScreenSize / 4.0;
    vec3 randomVec = normalize(texture(u_NoiseTexture, v_TexCoords * noiseScale).xyz);
    vec3 tangent = randomVec - normal * dot(randomVec, normal);
    float tLen = length(tangent);
    if (tLen < 0.0001)
    {
        tangent = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
        tangent = normalize(tangent - normal * dot(tangent, normal));
    }
    else
    {
        tangent /= tLen;
    }
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    int samplesToTake = clamp(u_SampleCount, 4, 32);

    for (int i = 0; i < samplesToTake; ++i)
    {
        // Yarım küre numunesini teğet uzaydan bakış uzayına taşı
        vec3 samplePos = TBN * u_Samples[i];
        samplePos = viewPos + samplePos * u_Radius;

        // Numuneyi ekran izdüşümüne (UV) dönüştür
        vec4 offset = vec4(samplePos, 1.0);
        offset = u_Projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // Ekran sınırları dışındaki numuneleri yoksay
        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0)
        {
            continue;
        }

        // Örneklenen yüzey noktasının gerçek derinliği
        float sampleDepth = getViewPos(offset.xy).z;

        // Mesafe toleransı: Uzak nesnelerin gereksiz gölgeleme yapmasını önle
        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / max(abs(viewPos.z - sampleDepth), 0.0001));

        // OpenGL bakış uzayında kamera -Z'ye bakar.
        // sampleDepth >= samplePos.z + u_Bias koşulu, yüzeyin numunenin önünde olduğunu gösterir.
        if (sampleDepth >= samplePos.z + u_Bias)
        {
            occlusion += rangeCheck;
        }
    }

    // [0, 1] aralığında görünürlük (1.0 = engellenmemiş, 0.0 = tamamen tıkalı)
    float ao = 1.0 - (occlusion / float(samplesToTake));
    ao = clamp(ao, 0.0, 1.0);

    // PS1 Retro Modu: Dithering ve kuantizasyon
    if (u_PS1Mode)
    {
        if (u_PS1Dithering)
        {
            float dither = (Bayer4x4(gl_FragCoord.xy) - 0.5) * (u_PS1DitherStrength * 0.35);
            ao += dither;
        }
        // 8 kademeli retro kuantizasyon
        ao = floor(ao * 8.0 + 0.5) / 8.0;
        ao = clamp(ao, 0.0, 1.0);
    }

    FragColor = vec4(ao, ao, ao, 1.0);
}

