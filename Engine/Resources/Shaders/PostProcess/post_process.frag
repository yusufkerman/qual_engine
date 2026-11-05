#version 330 core

/**
 * @file post_process.frag
 * @brief HDR Tone Mapping (None, Reinhard, ACES, AgX, Uncharted 2), sRGB Gamma Düzeltmesi ve Anti-Aliasing (FXAA 3.11 & TAA) shader'ı.
 *
 * u_PassMode == 0: TAA Temporal Reprojection + 3x3 YCoCg Variance Clipping (HDR uzayında)
 * u_PassMode == 1: Exposure + Tone Mapping + sRGB Gamma + FXAA 3.11 / TAA Contrast-Adaptive Sharpen (LDR çıktı)
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform int u_PassMode = 1; // 0: TAA Resolve, 1: ToneMap + FXAA/Sharpen

uniform sampler2D u_SceneTexture;
uniform sampler2D u_HistoryTexture;
uniform sampler2D u_SceneDepth;

uniform vec2 u_TexelSize;

// Tone Mapping Parametreleri
uniform int u_ToneMapOperator = 2; // 0: None (Clamp), 1: Reinhard, 2: ACES, 3: AgX, 4: Uncharted 2
uniform float u_ExposureEV = 0.0;
uniform float u_WhitePoint = 4.0;
uniform bool u_PS1Mode = false;

// Anti-Aliasing (FXAA & TAA) Parametreleri
uniform int u_AAMode = 2; // 0: Off, 1: FXAA, 2: TAA
uniform bool u_HasHistory = false;
uniform float u_TAABlend = 0.88;
uniform float u_TAASharpen = 0.25;
uniform mat4 u_InvViewProj;
uniform mat4 u_PrevViewProj;

// ============================================================================
// YARDIMCI RENK UZAYI VE TONE MAPPING FONKSİYONLARI
// ============================================================================

float Luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 RGBToYCoCg(vec3 c)
{
    return vec3(
         0.25 * c.r + 0.5 * c.g + 0.25 * c.b,
         0.50 * c.r             - 0.50 * c.b,
        -0.25 * c.r + 0.5 * c.g - 0.25 * c.b
    );
}

vec3 YCoCgToRGB(vec3 ycocg)
{
    float y  = ycocg.x;
    float co = ycocg.y;
    float cg = ycocg.z;
    return max(vec3(y + co - cg, y + cg, y - co - cg), vec3(0.0));
}

// 1. Reinhard (Extended White Point)
vec3 ToneMapReinhard(vec3 x, float whitePoint)
{
    float W2 = max(whitePoint * whitePoint, 1.0);
    return (x * (1.0 + x / W2)) / (1.0 + x);
}

// 2. ACES Filmic (Stephen Hill sRGB -> RRT+ODT Fit)
vec3 ToneMapACES(vec3 x)
{
    const mat3 acesInput = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );
    const mat3 acesOutput = mat3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );
    vec3 v = acesInput * x;
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return clamp(acesOutput * (a / b), 0.0, 1.0);
}

// 3. AgX (Troy Sobotka Minimal AgX + Punchy Look)
vec3 AgXDefaultContrastApprox(vec3 x)
{
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

vec3 ToneMapAgX(vec3 val)
{
    const mat3 agxInset = mat3(
        0.842479062253094,  0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772,  0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104
    );
    const mat3 agxOutset = mat3(
         1.19687900512017,   -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368,  1.15190312990417,   -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433,  1.15107367264116
    );

    val = agxInset * max(val, vec3(1e-10));
    const float minEv = -12.47393;
    const float maxEv = 4.026069;
    val = clamp(log2(val), minEv, maxEv);
    val = (val - minEv) / (maxEv - minEv);
    val = clamp(AgXDefaultContrastApprox(val), 0.0, 1.0);

    // Punchy / Golden Look (Doygunluğu ve gölge kontrastını koru)
    float luma = dot(val, vec3(0.2126, 0.7152, 0.0722));
    val = luma + (val - luma) * 1.18;
    val = pow(max(val, vec3(0.0)), vec3(1.12));

    val = agxOutset * val;
    return clamp(pow(max(val, vec3(0.0)), vec3(2.2)), 0.0, 1.0);
}

// 4. Uncharted 2 / Hable Filmic
vec3 Uncharted2Curve(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ToneMapUncharted2(vec3 x, float whitePoint)
{
    vec3 curr = Uncharted2Curve(x * 2.0);
    vec3 whiteScale = 1.0 / max(Uncharted2Curve(vec3(max(whitePoint, 1.5))), vec3(0.0001));
    return clamp(curr * whiteScale, 0.0, 1.0);
}

// HDR -> Tone Mapped + sRGB Gamma Corrected LDR [0, 1]
vec3 SampleAndToneMapLDR(vec2 uv)
{
    vec3 hdr = max(texture(u_SceneTexture, uv).rgb, vec3(0.0));
    if (u_PS1Mode)
    {
        return clamp(hdr, 0.0, 1.0);
    }

    hdr *= exp2(u_ExposureEV);

    vec3 ldr;
    if (u_ToneMapOperator == 0)
    {
        ldr = clamp(hdr, 0.0, 1.0);
    }
    else if (u_ToneMapOperator == 1)
    {
        ldr = ToneMapReinhard(hdr, u_WhitePoint);
    }
    else if (u_ToneMapOperator == 3)
    {
        ldr = ToneMapAgX(hdr);
    }
    else if (u_ToneMapOperator == 4)
    {
        ldr = ToneMapUncharted2(hdr, u_WhitePoint);
    }
    else // Default 2: ACES Filmic
    {
        ldr = ToneMapACES(hdr);
    }

    return pow(clamp(ldr, 0.0, 1.0), vec3(1.0 / 2.2));
}

// ============================================================================
// GEÇİŞ 0: TAA TEMPORAL RESOLVE (HALTON REPROJECTION + VARIANCE CLIPPING)
// ============================================================================
vec4 ExecuteTAAResolve()
{
    vec3 currentHDR = max(texture(u_SceneTexture, v_TexCoords).rgb, vec3(0.0));
    if (!u_HasHistory)
    {
        return vec4(currentHDR, 1.0);
    }

    float rawDepth = texture(u_SceneDepth, v_TexCoords).r;
    // Hareketli kenarlarda en yakın derinliği (3x3 closest depth dilation) seçerek kenar titremesini önle
    float closestDepth = rawDepth;
    vec2 closestUV = v_TexCoords;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 suv = v_TexCoords + vec2(float(x), float(y)) * u_TexelSize;
            float d = texture(u_SceneDepth, suv).r;
            if (d < closestDepth)
            {
                closestDepth = d;
                closestUV = suv;
            }
        }
    }

    // Dünya uzayı noktasından önceki kare ekran koordinatına (prevUV) reprojeksiyon
    vec4 ndc = vec4(closestUV * 2.0 - 1.0, closestDepth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_InvViewProj * ndc;
    worldPos /= max(worldPos.w, 0.00001);

    vec4 prevClip = u_PrevViewProj * vec4(worldPos.xyz, 1.0);
    vec2 prevNDC = prevClip.xy / max(prevClip.w, 0.00001);
    vec2 velocity = (prevNDC * 0.5 + 0.5) - closestUV;
    vec2 historyUV = v_TexCoords + velocity;

    // Ekran dışı piksellerde geçmişi reddet
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0)
    {
        return vec4(currentHDR, 1.0);
    }

    // 3x3 Komşuluk Varyans Kırpma (YCoCg uzayında Karis ağırlıklı ortalama ve standart sapma)
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    float totalW = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 suv = v_TexCoords + vec2(float(x), float(y)) * u_TexelSize;
            vec3 sRGB = max(texture(u_SceneTexture, suv).rgb, vec3(0.0));
            float w = 1.0 / (1.0 + Luminance(sRGB));
            vec3 sYCoCg = RGBToYCoCg(sRGB);
            m1 += sYCoCg * w;
            m2 += sYCoCg * sYCoCg * w;
            totalW += w;
        }
    }

    vec3 mu = m1 / max(totalW, 0.0001);
    vec3 sigma = sqrt(max((m2 / max(totalW, 0.0001)) - mu * mu, vec3(0.0)));

    // Hızlı kamera hareketlerinde ghosting'i sıfıra indirmek için varyans penceresini daralt
    float pixelSpeed = length(velocity / max(u_TexelSize, vec2(0.00001)));
    float gamma = mix(1.25, 0.75, clamp(pixelSpeed * 0.08, 0.0, 1.0));

    vec3 boxMin = mu - gamma * sigma;
    vec3 boxMax = mu + gamma * sigma;

    vec3 historyHDR = max(texture(u_HistoryTexture, historyUV).rgb, vec3(0.0));
    vec3 historyYCoCg = clamp(RGBToYCoCg(historyHDR), boxMin, boxMax);
    vec3 clippedHistoryHDR = YCoCgToRGB(historyYCoCg);

    // Parlaklık farkı ve hıza bağlı dinamik geri besleme (anti-flicker + anti-ghosting)
    float speedAtten = clamp(1.0 - pixelSpeed * 0.04, 0.65, 1.0);
    float blendFactor = clamp(u_TAABlend * speedAtten, 0.50, 0.96);

    float wCurr = 1.0 / (1.0 + Luminance(currentHDR));
    float wHist = 1.0 / (1.0 + Luminance(clippedHistoryHDR));

    vec3 resolvedHDR = (currentHDR * wCurr * (1.0 - blendFactor) + clippedHistoryHDR * wHist * blendFactor) /
                       max(wCurr * (1.0 - blendFactor) + wHist * blendFactor, 0.00001);

    return vec4(max(resolvedHDR, vec3(0.0)), 1.0);
}

// ============================================================================
// GEÇİŞ 1: FXAA 3.11 (LDR ÜZERİNDE) & TAA KESKİNLEŞTİRME
// ============================================================================
vec3 ExecuteFXAA()
{
    vec3 rgbM  = SampleAndToneMapLDR(v_TexCoords);
    vec3 rgbNW = SampleAndToneMapLDR(v_TexCoords + vec2(-1.0, -1.0) * u_TexelSize);
    vec3 rgbNE = SampleAndToneMapLDR(v_TexCoords + vec2( 1.0, -1.0) * u_TexelSize);
    vec3 rgbSW = SampleAndToneMapLDR(v_TexCoords + vec2(-1.0,  1.0) * u_TexelSize);
    vec3 rgbSE = SampleAndToneMapLDR(v_TexCoords + vec2( 1.0,  1.0) * u_TexelSize);

    const vec3 lumaVec = vec3(0.299, 0.587, 0.114);
    float lumaM  = dot(rgbM,  lumaVec);
    float lumaNW = dot(rgbNW, lumaVec);
    float lumaNE = dot(rgbNE, lumaVec);
    float lumaSW = dot(rgbSW, lumaVec);
    float lumaSE = dot(rgbSE, lumaVec);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    // Kontrast eşiği altındaysa kenar yoktur; doğrudan (varsa sharpen ile) döndür
    if (lumaRange < max(0.0312, lumaMax * 0.125))
    {
        return rgbM;
    }

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * 0.125), 1.0 / 128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * u_TexelSize;

    vec3 rgbA = 0.5 * (
        SampleAndToneMapLDR(v_TexCoords + dir * (1.0 / 3.0 - 0.5)) +
        SampleAndToneMapLDR(v_TexCoords + dir * (2.0 / 3.0 - 0.5))
    );
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        SampleAndToneMapLDR(v_TexCoords + dir * -0.5) +
        SampleAndToneMapLDR(v_TexCoords + dir *  0.5)
    );

    float lumaB = dot(rgbB, lumaVec);
    vec3 fxaaResult = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB;

    if (u_TAASharpen > 0.001)
    {
        vec3 avgDiag = 0.25 * (rgbNW + rgbNE + rgbSW + rgbSE);
        fxaaResult = clamp(fxaaResult + (fxaaResult - avgDiag) * (u_TAASharpen * 0.5), 0.0, 1.0);
    }
    return fxaaResult;
}

void main()
{
    if (u_PassMode == 0)
    {
        FragColor = ExecuteTAAResolve();
        return;
    }

    if (u_PS1Mode)
    {
        FragColor = vec4(SampleAndToneMapLDR(v_TexCoords), 1.0);
        return;
    }

    if (u_AAMode == 1) // FXAA 3.11
    {
        FragColor = vec4(ExecuteFXAA(), 1.0);
        return;
    }

    vec3 centerLDR = SampleAndToneMapLDR(v_TexCoords);

    // TAA açıkken veya keskinleştirme istendiğinde Contrast-Adaptive Sharpen uygula
    if (u_AAMode == 2 && u_TAASharpen > 0.001)
    {
        vec3 nN = SampleAndToneMapLDR(v_TexCoords + vec2( 0.0,  1.0) * u_TexelSize);
        vec3 nS = SampleAndToneMapLDR(v_TexCoords + vec2( 0.0, -1.0) * u_TexelSize);
        vec3 nE = SampleAndToneMapLDR(v_TexCoords + vec2( 1.0,  0.0) * u_TexelSize);
        vec3 nW = SampleAndToneMapLDR(v_TexCoords + vec2(-1.0,  0.0) * u_TexelSize);
        vec3 neighborAvg = 0.25 * (nN + nS + nE + nW);
        centerLDR = clamp(centerLDR + (centerLDR - neighborAvg) * (u_TAASharpen * 0.85), 0.0, 1.0);
    }

    FragColor = vec4(centerLDR, 1.0);
}
