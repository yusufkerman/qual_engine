#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 baseColor;
uniform bool useTexture;
uniform sampler2D diffuseTex;

uniform vec3 lightPos;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vWorldPos);

    float diff = max(dot(N, L), 0.0);
    float ambient = 0.25;

    float light = ambient + diff * 0.75;

    vec3 albedo = baseColor;

    if (useTexture)
    {
        albedo = texture(diffuseTex, vUV).rgb;
    }

    vec3 color = albedo * light;

    FragColor = vec4(color, 1.0);
}