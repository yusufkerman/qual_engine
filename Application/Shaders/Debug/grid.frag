#version 330 core

out vec4 FragColor;

in vec2 uv;

uniform mat4 invViewProj;
uniform mat4 viewProj; // 🔥 DERİNLİK HESABI İÇİN EKLENDİ
uniform vec3 cameraPos;

// Kalınlığı katman ölçeğine göre dengeleyen fonksiyon
float analyticalGrid(vec2 p, float scale, float dist)
{
    vec2 coord = p / scale;
    
    // 🔥 ANA ÇİZGİLERİN KALINLAŞMASINI ENGELLEYEN FORMÜL:
    // Kalınlığı scale değerine bölerek, büyük gridlerin (scale büyüdükçe) 
    // devasa kalınlıklara ulaşmasını engelliyoruz.
    float baseThickness = 0.04; 
    float thickness = baseThickness * (1.0 + pow(dist, 1.1) * 0.02); 
    
    // Büyük ölçekli gridlerin çizgilerini alt katmana göre daha zarif yapar
    thickness = min(thickness, scale * 0.05); 

    vec2 gridReal = abs(fract(coord - 0.5) - 0.5) / (thickness / scale);
    vec2 line = smoothstep(1.0, 0.0, gridReal);
    
    return max(line.x, line.y);
}

void main()
{
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 near = invViewProj * vec4(ndc, 0.0, 1.0);
    vec4 far  = invViewProj * vec4(ndc, 1.0, 1.0);

    near /= near.w;
    far  /= far.w;

    vec3 dir = normalize(far.xyz - near.xyz);

    float t = -near.y / dir.y;
    if (t < 0.0) discard;

    vec3 worldPos = near.xyz + dir * t;
    vec2 xz = worldPos.xz;

    float dist = length(worldPos - cameraPos);

    // 🔥 1. DİĞER MESHLERİN ARKASINDA RENDER OLMA FIXI (Depth Yazımı)
    // Elle hesapladığımız dünya koordinatını tekrar View-Projection ile çarpıp
    // ekran kartının derinlik testine (Z-Buffer) sokuyoruz.
    vec4 clipPos = viewProj * vec4(worldPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w; 

    // 5 tabanlı logaritmik ölçeklendirme
    float logDist = log2(max(dist, 1.0)) / log2(3.0);
    float lod1 = floor(logDist);
    float lod2 = lod1 + 1.0;

    float scale1 = pow(3.0, lod1);
    float scale2 = pow(3.0, lod2);

    float g1 = analyticalGrid(xz, scale1, dist);
    float g2 = analyticalGrid(xz, scale2, dist);

    float f = fract(logDist);
    float blend = smoothstep(0.0, 1.0, f);

    float grid = mix(g1, g2, blend);

    float horizonFade = 1.0 - smoothstep(1000.0, 3000.0, dist);
    grid *= horizonFade;

    grid = clamp(grid, 0.0, 1.0);

    FragColor = vec4(vec3(0.35), grid);
}