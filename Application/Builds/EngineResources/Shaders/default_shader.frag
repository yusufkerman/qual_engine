#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in mat3 vTBN;
in vec4 vFragPosLightSpace;

out vec4 FragColor;

uniform vec3 baseColor;
uniform bool useTexture;
uniform sampler2D diffuseTex;
uniform vec2 u_TextureScale; 

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength;

uniform float specularStrength;
uniform float shininess;

//  Kameranın dünyadaki pozisyonu (C++ tarafında her render'da set edilmeli)
uniform vec3 viewPos;
uniform sampler2D shadowMap;

uniform int u_RenderMode;       // 0: Opaque, 1: Cutout
uniform float u_AlphaThreshold;

// ============================================================================
// FOG
// ============================================================================
uniform bool u_FogEnabled;
uniform vec3 u_FogColor;
uniform float u_FogDensity;
// ============================================================================

// ============================================================================
// UBO YAPISI (std140) - HİZALAMA (Tam 80 Byte / 5 adet vec4)
// ============================================================================
struct Light
{
    int   type;
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
// ============================================================================

// Gölge Hesaplama Fonksiyonu
float CalculateShadow(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float bias = max(0.0015 * (1.0 - dot(N,L)), 0.0003);

    vec2 texelSize = 1.0 / textureSize(shadowMap,0);

    float shadow = 0.0;

    for(int x=-1;x<=1;x++)
    {
        for(int y=-1;y<=1;y++)
        {
            float depth = texture(
                shadowMap,
                projCoords.xy + vec2(x,y)*texelSize
            ).r;

            shadow +=
                (projCoords.z-bias>depth)
                ?1.0:0.0;
        }
    }

    shadow/=9.0;

    return shadow;
}
void main()
{
    
    vec2 tiledUV = vUV * u_TextureScale;

    vec4 texColor = useTexture ? texture(diffuseTex, tiledUV) : vec4(baseColor, 1.0);
    if (u_RenderMode == 1)
    {
        if (texColor.a < u_AlphaThreshold)
        {
            discard; // Bu komut çalıştığı an shader aşağıya devam etmez, pikseller şeffaf olur!
        }
    }

    vec3 albedo = texColor.rgb;
    vec3 N = normalize(vNormal);

    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, tiledUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }


    vec3 result = vec3(0.0);
    float ambient = 0.15;

    result += albedo * ambient;

    //  Pikselden kameraya doğru giden ortak görüş vektörü (V)
    vec3 V = normalize(viewPos - vWorldPos);

    for (int i = 0; i < lightCount && i < 64; i++)
    {
        Light l = lights[i];

        vec3 L;
        float attenuation = 1.0;

        if (l.type == 0) // DIRECTIONAL
        {
            L = normalize(-l.direction);
        }
        else if (l.type == 1) // POINT
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);

            attenuation = 1.0 / (1.0 + (dist / l.range) * (dist / l.range));
        }
        else // SPOT
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = toLight / dist;

            vec3 lightDir = normalize(l.direction);
            float theta = dot(-lightDir, L);

            float inner = cos(radians(l.inner_cone));
            float outer = cos(radians(l.outer_cone));

            float spot = smoothstep(outer, inner, theta);

            float distanceAtt = 1.0 / (1.0 + (dist * dist) / (l.range * l.range));
            attenuation *= distanceAtt * spot; 
        }

        // 1. Diffuse (Dağınık Aydınlatma)
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = albedo * l.color * l.intensity * diff * attenuation;

        // Blinn-Phong Specular (Parlama) Hesaplaması
        vec3 specular = vec3(0.0);
        if (diff > 0.0) // Eğer yüzey ışık alıyorsa parlayabilir
        {
            // Işık vektörü (L) ile Görüş vektörünün (V) tam ortasındaki vektör (Halfway Vector)
            vec3 H = normalize(L + V);

            // Parlama keskinliği (Shininnes/Shininess Exponent). 
            // 32.0 plastik gibidir, 64-128 metallere doğru gider. Şimdilik sabit veriyoruz.
            float specPower = pow(max(dot(N, H), 0.0), shininess);

            // Işığın kendi rengi ve şiddetiyle parlamayı çarpıyoruz
            specular = l.color * l.intensity * specPower * attenuation * specularStrength;
        }

        float shadow = 0.0;
        if (l.type == 0) 
        {
            shadow = CalculateShadow(vFragPosLightSpace, N, L);
        }

        // Nihai sonuca diffuse'un yanına parlama (specular) efektini de ekliyoruz
        result += (1.0 - shadow) * (diffuse + specular);    
    }

    // ============================================================================
    // EXPONENTIAL FOG
    // ============================================================================
    if (u_FogEnabled)
    {
        // Pikselin kameraya olan gerçek dünyadaki mesafesi
        float distance = length(viewPos - vWorldPos);

        // Exponential (Üstel) Sis Formülü: e^(-density * distance)
        float fogFactor = exp(-u_FogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        // Işıklandırılmış rengi sis rengi ile mix et
        result = mix(u_FogColor, result, fogFactor);
    }
    // ============================================================================

    float finalAlpha = (u_RenderMode == 2) ? texColor.a : 1.0;

    FragColor = vec4(result, finalAlpha);
}