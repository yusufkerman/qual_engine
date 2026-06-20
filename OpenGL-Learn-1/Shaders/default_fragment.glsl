#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
in mat3 vTBN;

out vec4 FragColor;

uniform vec3 baseColor;
uniform bool useTexture;
uniform sampler2D diffuseTex;

uniform bool useNormalMap;
uniform sampler2D normalTex;
uniform float normalStrength;

struct Light
{
    int type;

    vec3 position;
    vec3 direction;

    vec3 color;

    float intensity;

    float range;

    float inner_cone;
    float outer_cone;
};

uniform int lightCount;
uniform Light lights[64];

void main()
{
    vec3 N = normalize(vNormal);

    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalTex, vUV).rgb * 2.0 - 1.0;
        tangentNormal.xy *= normalStrength;
        N = normalize(vTBN * normalize(tangentNormal));
    }

    vec3 albedo = useTexture ? texture(diffuseTex, vUV).rgb : baseColor;

    vec3 result = vec3(0.0);
    float ambient = 0.15;

    result += albedo * ambient;

    for (int i = 0; i < lightCount; i++)
    {
        Light l = lights[i];

        vec3 L;
        float attenuation = 1.0;

        // -------------------------
        // DIRECTIONAL
        // -------------------------
        if (l.type == 0)
        {
            L = normalize(-l.direction);
        }

        // -------------------------
        // POINT
        // -------------------------
        else if (l.type == 1)
        {
            vec3 toLight = l.position - vWorldPos;
            float dist = length(toLight);
            L = (dist > 0.0001) ? (toLight / dist) : vec3(0.0);

            attenuation = 1.0 / (1.0 + (dist / l.range) * (dist / l.range));
        }

        // -------------------------
        // SPOT
        // -------------------------
        else
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

        float diff = max(dot(N, L), 0.0);

        vec3 diffuse =
            albedo *
            l.color *
            l.intensity *
            diff *
            attenuation;

        result += diffuse;
    }

    FragColor = vec4(result, 1.0);
}