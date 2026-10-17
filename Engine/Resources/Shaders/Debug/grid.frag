#version 330 core

out vec4 FragColor;

in vec2 uv;

uniform mat4 invViewProj;
uniform mat4 viewProj;
uniform vec3 cameraPos;
uniform float u_GridOpacity;
uniform int u_ShowAxes;

// Ekran uzayı türevleri (fwidth) ile 1-piksel keskinliğinde anti-aliased ızgara hesaplaması
vec4 computeGridLevel(vec2 xz, float cellSize, float lineScale)
{
    vec2 coord = xz / cellSize;
    vec2 deriv = fwidth(coord);
    // Türev sıfıra çok yakınsa bölme hatasını önle
    deriv = max(deriv, vec2(1e-6));

    vec2 gridDist = abs(fract(coord - 0.5) - 0.5) / (deriv * lineScale);
    float line = min(gridDist.x, gridDist.y);
    float alpha = 1.0 - min(line, 1.0);

    return vec4(vec3(0.55), alpha);
}

void main()
{
    vec2 ndc = uv * 2.0 - 1.0;

    // OpenGL NDC derinlik aralığı [-1, 1]
    vec4 nearPoint = invViewProj * vec4(ndc, -1.0, 1.0);
    vec4 farPoint  = invViewProj * vec4(ndc,  1.0, 1.0);

    nearPoint /= nearPoint.w;
    farPoint  /= farPoint.w;

    vec3 rayDir = farPoint.xyz - nearPoint.xyz;
    if (abs(rayDir.y) < 1e-6)
        discard;

    float t = -nearPoint.y / rayDir.y;
    if (t <= 0.0)
        discard;

    vec3 worldPos = nearPoint.xyz + rayDir * t;
    vec2 xz = worldPos.xz;

    // Derinlik tamponu (Z-buffer) değerini [0, 1] aralığına dönüştürerek yaz
    vec4 clipPos = viewProj * vec4(worldPos, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    if (ndcDepth < -1.0 || ndcDepth > 1.0)
        discard;

    gl_FragDepth = clamp(ndcDepth * 0.5 + 0.5, 0.0, 1.0);

    // Kamera yüksekliği ve yatay mesafeye göre dinamik LOD ölçekleme
    float camHeight = max(abs(cameraPos.y), 1.5);
    float logLevel  = log(camHeight) / log(10.0);
    float baseLevel = floor(logLevel);
    float levelFrac = fract(logLevel);

    float cellSmall  = pow(10.0, baseLevel - 1.0);
    float cellMedium = pow(10.0, baseLevel);
    float cellLarge  = pow(10.0, baseLevel + 1.0);

    vec4 gSmall  = computeGridLevel(xz, cellSmall,  1.0);
    vec4 gMedium = computeGridLevel(xz, cellMedium, 1.25);
    vec4 gLarge  = computeGridLevel(xz, cellLarge,  1.6);

    // Küçük hücreler kamera yükseldikçe yumuşakça kaybolur
    float smallFade = 1.0 - smoothstep(0.1, 0.85, levelFrac);
    float smallAlpha  = gSmall.a * 0.28 * smallFade;
    float mediumAlpha = gMedium.a * mix(0.55, 0.32, levelFrac);
    float largeAlpha  = gLarge.a * 0.68;

    float combinedAlpha = max(smallAlpha, max(mediumAlpha, largeAlpha));
    vec3 gridColor = mix(vec3(0.45), vec3(0.68), step(smallAlpha, mediumAlpha));

    // X (Kırmızı) ve Z (Mavi) ana eksen çizgileri
    if (u_ShowAxes != 0)
    {
        vec2 worldDeriv = max(fwidth(xz), vec2(1e-5));
        float zAxisDist = abs(xz.x) / (worldDeriv.x * 1.6); // X = 0 doğrusu -> Z ekseni (Mavi)
        float xAxisDist = abs(xz.y) / (worldDeriv.y * 1.6); // Z = 0 doğrusu -> X ekseni (Kırmızı)

        float zAxisAlpha = 1.0 - min(zAxisDist, 1.0);
        float xAxisAlpha = 1.0 - min(xAxisDist, 1.0);

        if (xAxisAlpha > 0.01)
        {
            gridColor = mix(gridColor, vec3(0.92, 0.24, 0.26), xAxisAlpha);
            combinedAlpha = max(combinedAlpha, xAxisAlpha * 0.90);
        }
        if (zAxisAlpha > 0.01)
        {
            gridColor = mix(gridColor, vec3(0.24, 0.52, 0.96), zAxisAlpha);
            combinedAlpha = max(combinedAlpha, zAxisAlpha * 0.90);
        }
    }

    // Ufuk çizgisi (mesafe) ve eğik bakış açısı sönümlemesi (Anti-Moire)
    float dist = length(worldPos - cameraPos);
    float fadeRadius = max(120.0, camHeight * 35.0);
    float distFade = 1.0 - smoothstep(fadeRadius * 0.25, fadeRadius, dist);

    vec3 viewVec = normalize(worldPos - cameraPos);
    float angleFade = smoothstep(0.02, 0.20, abs(viewVec.y));

    float opacity = (u_GridOpacity > 0.0) ? u_GridOpacity : 1.0;
    float finalAlpha = combinedAlpha * distFade * angleFade * opacity;

    if (finalAlpha < 0.005)
        discard;

    FragColor = vec4(gridColor, clamp(finalAlpha, 0.0, 1.0));
}