#version 330 core

/**
 * @file ssao_composite.frag
 * @brief Hesaplanmış ve filtrelenmiş AO (SSAO/GTAO) ile SSGI dolaylı ışığını sahneye birleştirir.
 *
 * KURAL: AO yalnızca dolaylı (ambient / indirect) ışığa uygulanır, doğrudan (direct) güneş ışığını karartmaz!
 * Sahne HDR tamponunun alfa kanalında (u_SceneColor.a) saklanan dolaylı ışık oranı (indirectWeight = L_amb / (L_amb + L_direct))
 * kullanılarak doğrudan ışık (%100 korunur) ve dolaylı ışık ayrıştırılır; AO ve SSGI yalnızca dolaylı bileşene uygulanır.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_SSAOTexture;
uniform sampler2D u_SceneDepth;
uniform sampler2D u_SceneColor;
uniform sampler2D u_SSGITexture;

uniform float u_Intensity;
uniform bool u_ShowDebug;
uniform bool u_EnableAO = true;
uniform bool u_EnableSSGI = false;

void main()
{
    vec4 sceneSample = texture(u_SceneColor, v_TexCoords);
    vec3 totalHDR = sceneSample.rgb;
    float indirectWeight = clamp(sceneSample.a, 0.0, 1.0);

    float rawDepth = texture(u_SceneDepth, v_TexCoords).r;
    if (rawDepth >= 1.0 - 0.00005)
    {
        FragColor = vec4(totalHDR, 1.0);
        return;
    }

    float ao = u_EnableAO ? texture(u_SSAOTexture, v_TexCoords).r : 1.0;

    if (u_ShowDebug)
    {
        FragColor = vec4(ao, ao, ao, 1.0);
        return;
    }

    float aoFactor = u_EnableAO ? clamp(mix(1.0, ao, u_Intensity), 0.04, 1.0) : 1.0;

    // 1. Doğrudan (Direct) ve Dolaylı (Indirect/Ambient) ışığı ayrıştır:
    // AO yalnızca dolaylı ışığa uygulanır; doğrudan güneş alan yüzeyler kararmaz!
    vec3 directLight = totalHDR * (1.0 - indirectWeight);
    vec3 indirectLight = totalHDR * indirectWeight;
    vec3 occludedIndirect = indirectLight * aoFactor;

    // 2. Ekran Uzayı Tek-Sekme Dolaylı Işık (SSGI Bounce Light):
    vec3 bounceLight = vec3(0.0);
    if (u_EnableSSGI)
    {
        vec3 ssgiIrradiance = texture(u_SSGITexture, v_TexCoords).rgb;
        // Alıcı yüzeyin kendi renk tonunu (albedo chromaticity) tahmin et ki seken ışık yüzey rengiyle çarpılsın
        float maxCh = max(totalHDR.r, max(totalHDR.g, totalHDR.b));
        vec3 surfaceTint = (maxCh > 0.001) ? clamp(totalHDR / maxCh, vec3(0.18), vec3(1.0)) : vec3(0.75);
        bounceLight = ssgiIrradiance * surfaceTint * aoFactor;
    }

    vec3 finalHDR = directLight + occludedIndirect + bounceLight;
    FragColor = vec4(finalHDR, indirectWeight);
}
