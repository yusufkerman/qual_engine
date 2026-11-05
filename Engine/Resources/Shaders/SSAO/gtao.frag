#version 330 core

/**
 * @file gtao.frag
 * @brief Ground Truth Ambient Occlusion (GTAO) - Jimenez et al. (Activision 2016) & Intel XeGTAO.
 *
 * Özellikler:
 * - Projeksiyon katsayıları ile doğrudan hızlı analitik view-space rekonstrüksiyonu.
 * - Derinlik sıçramalarını (edge discontinuity) reddeden 4-örnekli derinlik duyarlı yüzey normali.
 * - Perspektif düzeltmeli 3D dilim (slice) ve ortogonal eksen uzayı.
 * - Cosine-space ufuk taraması (dot product ile, açı taşmaları ve zıplamaları sıfırlanmış).
 * - Analitik kosinüs ağırlıklı ufuk integrali ve açıya bağlı unoccluded normalizasyonu.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_SceneDepth;

uniform mat4 u_Projection;
uniform mat4 u_InvProjection;
uniform vec2 u_ScreenSize;

// GTAO Parametreleri
uniform float u_Radius;         // Dünya/bakış uzayı arama yarıçapı (Metre)
uniform float u_Falloff;        // Mesafe sönümleme katsayısı
uniform int u_SampleSteps;      // Her yöndeki adım sayısı (örn: 2 - 6)
uniform int u_Directions;       // Dilim yönü sayısı (örn: 1 - 4)
uniform float u_Intensity;      // Gölge gücü / kontrast üssü
uniform float u_Thickness;      // İnce nesneler için arka yüzey kalınlık toleransı (Metre)

#define PI 3.14159265358979323846
#define HALF_PI 1.5707963267948966

/**
 * @brief Jorge Jimenez - Interleaved Gradient Noise (IGN)
 * Piksel bazında uzamsal yüksek frekanslı deterministik gürültü üretir.
 */
float InterleavedGradientNoise(vec2 screenPos)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

/**
 * @brief Projeksiyon matrisi katsayıları ile 4x4 çarpım yapmadan doğrudan view-space pozisyon hesabı.
 */
vec3 getViewPosFast(vec2 uv)
{
    float rawDepth = texture(u_SceneDepth, uv).r;
    float ndcZ = rawDepth * 2.0 - 1.0;
    
    // OpenGL perspective: clipZ = P[2][2]*z + P[3][2], clipW = -z
    // ndcZ = clipZ / clipW => zView = -P[3][2] / (ndcZ + P[2][2])
    float denom = ndcZ + u_Projection[2][2];
    denom = (abs(denom) > 0.00001) ? denom : 0.00001;
    float zView = -u_Projection[3][2] / denom;
    
    float xView = (uv.x * 2.0 - 1.0) * (-zView) / u_Projection[0][0];
    float yView = (uv.y * 2.0 - 1.0) * (-zView) / u_Projection[1][1];
    return vec3(xView, yView, zView);
}

/**
 * @brief 2x2 quad dFdx/dFdy bloklanmasını ve kenar halolarını engelleyen derinlik duyarlı normal rekonstrüksiyonu.
 */
vec3 reconstructNormal(vec2 uv, vec3 viewPos)
{
    vec2 texelSize = 1.0 / u_ScreenSize;

    vec3 pR = getViewPosFast(uv + vec2(texelSize.x, 0.0));
    vec3 pL = getViewPosFast(uv - vec2(texelSize.x, 0.0));
    vec3 pU = getViewPosFast(uv + vec2(0.0, texelSize.y));
    vec3 pD = getViewPosFast(uv - vec2(0.0, texelSize.y));

    // Kenar sıçraması olmayan (derinlik farkı daha küçük olan) tarafı seç
    vec3 dx = (abs(pR.z - viewPos.z) < abs(pL.z - viewPos.z)) ? (pR - viewPos) : (viewPos - pL);
    vec3 dy = (abs(pU.z - viewPos.z) < abs(pD.z - viewPos.z)) ? (pU - viewPos) : (viewPos - pD);

    vec3 N = cross(dx, dy);
    float nLen = length(N);
    if (nLen < 0.0001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    N /= nLen;

    // Normalin kameraya dönük olmasını garantiye al
    if (dot(N, -normalize(viewPos)) < 0.0)
    {
        N = -N;
    }
    return N;
}

void main()
{
    float rawDepth = texture(u_SceneDepth, v_TexCoords).r;

    // Gökyüzü veya sonsuz derinlikteki pikseller için ortam örtmesi uygulama
    if (rawDepth >= 1.0 - 0.00005)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 viewPos = getViewPosFast(v_TexCoords);
    vec3 normal = reconstructNormal(v_TexCoords, viewPos);

    // Kameraya doğru bakan birim vektör (OpenGL view-space'te kamera orijindedir)
    vec3 V = normalize(-viewPos);

    // Ekran uzayındaki yarıçapı projeksiyon matrisi ölçeğine göre piksel cinsinden hesapla
    float projScale = u_Projection[1][1] * 0.5 * u_ScreenSize.y;
    float maxScreenRadius = u_ScreenSize.y * 0.35;
    float screenRadius = clamp((u_Radius * projScale) / max(-viewPos.z, 0.1), 3.0, maxScreenRadius);

    // Uzamsal gürültü: Açı döndürmesi ve adım ötelemesi birbirinden de-korele edilir
    float spatialAngle = InterleavedGradientNoise(gl_FragCoord.xy) * PI;
    float stepJitter = fract(InterleavedGradientNoise(gl_FragCoord.xy * 0.5 + vec2(0.33, 0.67)));

    int numDirections = clamp(u_Directions, 1, 6);
    int numSteps = clamp(u_SampleSteps, 2, 8);
    float thickness = (u_Thickness > 0.01) ? u_Thickness : max(0.5, u_Radius * 0.4);
    float falloffDist = max(0.01, u_Radius * max(0.2, u_Falloff));

    float totalVisibility = 0.0;
    float totalWeight = 0.0;

    for (int d = 0; d < numDirections; ++d)
    {
        float phi = (float(d) + spatialAngle / PI) * (PI / float(numDirections));
        vec2 dir = vec2(cos(phi), sin(phi));

        // 1. 2D ekran yönünü FOV ve en-boy oranına göre 3D view-space yönüne aç (Unproject)
        vec3 directionVec = vec3(dir.x / u_Projection[0][0], dir.y / u_Projection[1][1], 0.0);
        
        // 2. Bakış vektörüne dik ortogonal dilim vektörü
        vec3 orthoDirectionVec = directionVec - (dot(directionVec, V) * V);
        float orthoLen = length(orthoDirectionVec);
        if (orthoLen < 0.0001)
        {
            totalVisibility += 1.0;
            totalWeight += 1.0;
            continue;
        }
        orthoDirectionVec /= orthoLen;

        // 3. Dilim düzleminin normali (axisVec)
        vec3 axisVec = normalize(cross(orthoDirectionVec, V));

        // 4. Yüzey normalini dilim düzlemine izdüşür
        vec3 projectedNormalVec = normal - axisVec * dot(normal, axisVec);
        float projectedNormalVecLength = length(projectedNormalVec);
        if (projectedNormalVecLength < 0.0001)
        {
            totalVisibility += 1.0;
            totalWeight += 1.0;
            continue;
        }

        // İzdüşürülmüş normalin bakış vektörü V ile yaptığı açı n (XeGTAO)
        float signNorm = sign(dot(orthoDirectionVec, projectedNormalVec));
        float cosNorm = clamp(dot(projectedNormalVec, V) / projectedNormalVecLength, 0.0, 1.0);
        float n = signNorm * acos(cosNorm);

        // Ufuk kosinüslerini başlat (-1.0 = engellenmemiş taban düzlemi)
        float horizonCos0 = -1.0;
        float horizonCos1 = -1.0;

        // --- Dilim Boyunca İki Yöne (+dir ve -dir) Işın Yürütmesi ---
        for (int s = 1; s <= numSteps; ++s)
        {
            float alpha = (float(s) - 0.5 + stepJitter) / float(numSteps);
            float stepDist = alpha * alpha * screenRadius + 1.0;
            vec2 offset = dir * (stepDist / u_ScreenSize);

            // +dir (Yön 0)
            vec2 sampleUV0 = v_TexCoords + offset;
            if (sampleUV0.x >= 0.0 && sampleUV0.x <= 1.0 && sampleUV0.y >= 0.0 && sampleUV0.y <= 1.0)
            {
                vec3 samplePos0 = getViewPosFast(sampleUV0);
                vec3 delta0 = samplePos0 - viewPos;
                float dist0 = length(delta0);
                if (dist0 > 0.001)
                {
                    vec3 sampleHorizonVec0 = delta0 / dist0;
                    float shc0 = dot(sampleHorizonVec0, V);

                    float dFactor0 = clamp(dist0 / falloffDist, 0.0, 1.0);
                    float falloff0 = 1.0 - dFactor0 * dFactor0;

                    // Kalınlık toleransı: view-space derinlik farkı
                    float distBehind0 = max(0.0, viewPos.z - samplePos0.z);
                    float thicknessWeight0 = clamp(1.0 - distBehind0 / thickness, 0.0, 1.0);

                    shc0 = mix(-1.0, shc0, falloff0 * thicknessWeight0);
                    horizonCos0 = max(horizonCos0, shc0);
                }
            }

            // -dir (Yön 1)
            vec2 sampleUV1 = v_TexCoords - offset;
            if (sampleUV1.x >= 0.0 && sampleUV1.x <= 1.0 && sampleUV1.y >= 0.0 && sampleUV1.y <= 1.0)
            {
                vec3 samplePos1 = getViewPosFast(sampleUV1);
                vec3 delta1 = samplePos1 - viewPos;
                float dist1 = length(delta1);
                if (dist1 > 0.001)
                {
                    vec3 sampleHorizonVec1 = delta1 / dist1;
                    float shc1 = dot(sampleHorizonVec1, V);

                    float dFactor1 = clamp(dist1 / falloffDist, 0.0, 1.0);
                    float falloff1 = 1.0 - dFactor1 * dFactor1;

                    float distBehind1 = max(0.0, viewPos.z - samplePos1.z);
                    float thicknessWeight1 = clamp(1.0 - distBehind1 / thickness, 0.0, 1.0);

                    shc1 = mix(-1.0, shc1, falloff1 * thicknessWeight1);
                    horizonCos1 = max(horizonCos1, shc1);
                }
            }
        }

        // Ufuk açılarını kosinüslerden çıkar (Jimenez / XeGTAO)
        float h0 = -acos(clamp(horizonCos1, -1.0, 1.0));
        float h1 =  acos(clamp(horizonCos0, -1.0, 1.0));

        // Yarım küre sınırlarına kenetle
        h0 = n + clamp(h0 - n, -HALF_PI, HALF_PI);
        h1 = n + clamp(h1 - n, -HALF_PI, HALF_PI);

        // Analitik kosinüs ağırlıklı yay integrali (IntegrateArc)
        float sinN = sin(n);
        float iarc0 = (cosNorm + 2.0 * h0 * sinN - cos(2.0 * h0 - n)) * 0.25;
        float iarc1 = (cosNorm + 2.0 * h1 * sinN - cos(2.0 * h1 - n)) * 0.25;

        // Yüzey eğimine göre analitik unoccluded normalizasyonu: cosNorm + n * sinN
        float unoccluded = max(0.001, cosNorm + n * sinN);
        float sliceVisibility = clamp((iarc0 + iarc1) / unoccluded, 0.0, 1.0);

        totalVisibility += sliceVisibility * projectedNormalVecLength;
        totalWeight += projectedNormalVecLength;
    }

    float finalVisibility = (totalWeight > 0.0001) ? (totalVisibility / totalWeight) : 1.0;
    finalVisibility = clamp(finalVisibility, 0.0, 1.0);

    // Kontrast / Şiddet üssü uygulama
    if (u_Intensity > 0.01)
    {
        finalVisibility = pow(finalVisibility, u_Intensity);
    }

    FragColor = vec4(finalVisibility, finalVisibility, finalVisibility, 1.0);
}
