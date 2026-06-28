#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in mat3 vTBN;

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

// ============================================================================
// UBO YAPISI (std140) - HİZALAMA
// ============================================================================
struct Light
{
    int type;
    vec3 position; 
    float range;      

    vec3 direction;
    float intensity;  

    vec3 color;
    float inner_cone; 

    float outer_cone;
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

void main()
{
    vec2 tiledUV = vUV * u_TextureScale;
    vec3 N = normalize(vNormal);

    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, tiledUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }

    vec3 albedo = useTexture ? texture(diffuseTex, tiledUV).rgb : baseColor;

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

        // Nihai sonuca diffuse'un yanına parlama (specular) efektini de ekliyoruz
        result += diffuse + specular;
    }

    FragColor = vec4(result, 1.0);
}