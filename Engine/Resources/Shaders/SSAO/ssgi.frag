#version 330 core

/**
 * @file ssgi.frag
 * @brief Ekran Uzayı Tek-Sekme Dolaylı Yayılı Işıklandırma (Screen-Space Single-Bounce Indirect Diffuse GI - SSGI).
 *
 * Ekstra sahne objesi veya light probe gerektirmeden, yalnızca mevcut derinlik (u_SceneDepth) ve
 * HDR sahne rengi (u_SceneColor) tamponlarını kullanarak güneşli yüzeylerden (örn. Sponza zemini)
 * gölgedeki sütunlara, perdelere ve kemerlere seken dolaylı ışığı (color bleeding / bounce light) hesaplar.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_SceneDepth;
uniform sampler2D u_SceneColor;

uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform vec2 u_ScreenSize;

uniform float u_Radius = 4.5;
uniform int u_SampleCount = 6;
uniform float u_Intensity = 1.2;

const float PI = 3.14159265359;

// Derinlik değerinden View-Space konumunu yeniden oluşturur
vec3 getViewPos(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = u_InvProjection * clip;
    return view.xyz / max(view.w, 0.00001);
}

// Kenar süreksizliklerinde (obje silüetlerinde) halo oluşmaması için en küçük derinlik farkını seçen normal kurulumu
vec3 reconstructViewNormal(vec2 uv, vec3 P)
{
    vec2 texel = 1.0 / max(u_ScreenSize, vec2(1.0));

    float dL = texture(u_SceneDepth, uv - vec2(texel.x, 0.0)).r;
    float dR = texture(u_SceneDepth, uv + vec2(texel.x, 0.0)).r;
    float dD = texture(u_SceneDepth, uv - vec2(0.0, texel.y)).r;
    float dU = texture(u_SceneDepth, uv + vec2(0.0, texel.y)).r;

    vec3 pL = getViewPos(uv - vec2(texel.x, 0.0), dL);
    vec3 pR = getViewPos(uv + vec2(texel.x, 0.0), dR);
    vec3 pD = getViewPos(uv - vec2(0.0, texel.y), dD);
    vec3 pU = getViewPos(uv + vec2(0.0, texel.y), dU);

    vec3 dx = (abs(pR.z - P.z) < abs(P.z - pL.z)) ? (pR - P) : (P - pL);
    vec3 dy = (abs(pU.z - P.z) < abs(P.z - pD.z)) ? (pU - P) : (P - pD);

    vec3 n = cross(dx, dy);
    float len = length(n);
    return (len > 0.000001) ? (n / len) : vec3(0.0, 0.0, 1.0);
}

// Jimenez Interleaved Gradient Noise (IGN)
float IGN(vec2 pixelCoord)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(pixelCoord, magic.xy)));
}

void main()
{
    float centerDepth = texture(u_SceneDepth, v_TexCoords).r;
    if (centerDepth >= 1.0 - 0.00005)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 viewPos = getViewPos(v_TexCoords, centerDepth);
    vec3 viewNormal = reconstructViewNormal(v_TexCoords, viewPos);

    int numRays = clamp(u_SampleCount, 2, 16);
    int stepsPerRay = 3;

    float noise1 = IGN(gl_FragCoord.xy);
    float noise2 = IGN(gl_FragCoord.xy + vec2(17.3, 41.7));

    // View-space yarıçapının ekran uzayındaki karşılığı
    float projScale = abs(u_Projection[1][1]) * 0.5;
    float screenRadius = clamp((u_Radius * projScale) / max(-viewPos.z, 0.25), 0.02, 0.42);

    vec3 accumulatedGI = vec3(0.0);
    float validWeightSum = 0.0;

    for (int i = 0; i < numRays; ++i)
    {
        float angle = ((float(i) + noise1) / float(numRays)) * 2.0 * PI;
        vec2 dir = vec2(cos(angle), sin(angle));

        for (int s = 1; s <= stepsPerRay; ++s)
        {
            float stepFrac = (float(s) - 0.5 + noise2 * 0.5) / float(stepsPerRay);
            // Kuadratik adım dağılımı: yakın ve orta mesafeyi dengeli örnekle
            float r = mix(stepFrac, stepFrac * stepFrac, 0.45) * screenRadius;
            vec2 sampleUV = v_TexCoords + dir * r;

            if (sampleUV.x < 0.002 || sampleUV.x > 0.998 || sampleUV.y < 0.002 || sampleUV.y > 0.998)
            {
                continue;
            }

            float sDepth = texture(u_SceneDepth, sampleUV).r;
            if (sDepth >= 1.0 - 0.00005)
            {
                continue;
            }

            vec3 sPos = getViewPos(sampleUV, sDepth);
            vec3 diff = sPos - viewPos;
            float dist = length(diff);

            if (dist < 0.02 || dist > u_Radius * 1.35)
            {
                continue;
            }

            vec3 L = diff / dist;
            // Alıcı yüzeyin ön yarım küresinde mi? (Lambert kosinüs ağırlığı)
            float NdotL = max(dot(viewNormal, L) - 0.04, 0.0);
            if (NdotL <= 0.001)
            {
                continue;
            }

            // Mesafe ve kalınlık sönümlemesi
            float distFalloff = clamp(1.0 - (dist / (u_Radius * 1.25)), 0.0, 1.0);
            distFalloff *= distFalloff;

            vec3 sampleRadiance = texture(u_SceneColor, sampleUV).rgb;
            // Firefly (aşırı parlak tekil piksel) bastırma (Karis benzeri yumuşak tavan)
            float luma = dot(sampleRadiance, vec3(0.2126, 0.7152, 0.0722));
            vec3 clampedRadiance = sampleRadiance / (1.0 + luma * 0.22);

            float weight = NdotL * distFalloff;
            accumulatedGI += clampedRadiance * weight;
            validWeightSum += 1.0;
        }
    }

    vec3 finalGI = (validWeightSum > 0.0) ? (accumulatedGI / float(numRays * stepsPerRay)) * 3.2 * u_Intensity : vec3(0.0);
    FragColor = vec4(max(finalGI, vec3(0.0)), 1.0);
}
