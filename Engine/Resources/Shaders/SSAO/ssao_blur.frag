#version 330 core

/**
 * @file ssao_blur.frag
 * @brief SSAO / GTAO ve SSGI için kenar koruyucu (Bilateral) bulanıklaştırma shader'ı.
 *
 * Derinlik farkını dikkate alarak geometri silüetlerini keskin tutar ve hem tek kanallı AO
 * hem de 3 kanallı RGB SSGI sekme ışığı gürültüsünü temizler.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_SSAOTexture;
uniform sampler2D u_SceneDepth;
uniform vec2 u_TexelSize;
uniform bool u_PS1Mode;
uniform bool u_EnableBlur;
uniform float u_BlurStride = 1.0;

void main()
{
    if (u_PS1Mode || !u_EnableBlur)
    {
        FragColor = texture(u_SSAOTexture, v_TexCoords);
        return;
    }

    float centerDepth = texture(u_SceneDepth, v_TexCoords).r;
    if (centerDepth >= 1.0 - 0.00005)
    {
        FragColor = texture(u_SSAOTexture, v_TexCoords);
        return;
    }

    vec4 result = vec4(0.0);
    float totalWeight = 0.0;
    float stride = max(u_BlurStride, 1.0);

    // 5x5 Derinlik-duyarlı Bilateral filtre
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * u_TexelSize * stride;
            vec2 sampleUV = v_TexCoords + offset;

            float sampleDepth = texture(u_SceneDepth, sampleUV).r;
            vec4 sampleVal = texture(u_SSAOTexture, sampleUV);

            float spatialWeight = exp(-float(x * x + y * y) / 4.5);
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = 1.0 / (1.0 + depthDiff * 2200.0);
            float weight = spatialWeight * depthWeight;

            result += sampleVal * weight;
            totalWeight += weight;
        }
    }

    FragColor = (totalWeight > 0.0001) ? (result / totalWeight) : texture(u_SSAOTexture, v_TexCoords);
}
