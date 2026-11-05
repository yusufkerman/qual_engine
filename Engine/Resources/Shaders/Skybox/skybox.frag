#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform bool u_LinearHDR = true;
uniform int u_ToneMapMode = 2;
uniform bool u_FogEnabled = false;
uniform vec3 u_FogColor = vec3(0.5, 0.6, 0.7);
uniform float u_FogDensity = 0.01;

void main()
{
    vec3 envColor = texture(skybox, TexCoords).rgb;

    if (u_LinearHDR)
    {
        envColor = pow(max(envColor, vec3(0.0)), vec3(2.2));
        if (u_ToneMapMode > 0)
        {
            float lum = dot(envColor, vec3(0.2126, 0.7152, 0.0722));
            envColor *= (1.0 + 0.35 * smoothstep(0.4, 1.0, lum));
        }
    }

    if (u_FogEnabled)
    {
        vec3 dir = normalize(TexCoords);
        float horizonFactor = 1.0 - clamp(abs(dir.y), 0.0, 1.0);
        float fogAmount = clamp(pow(horizonFactor, 4.0) * clamp(u_FogDensity * 45.0, 0.0, 0.92), 0.0, 0.92);
        vec3 targetFog = u_LinearHDR ? pow(max(u_FogColor, vec3(0.0)), vec3(2.2)) : u_FogColor;
        envColor = mix(envColor, targetFog, fogAmount);
    }

    FragColor = vec4(envColor, 0.0);
}
