#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

// PS1 Retro Vertex Snapping Grid (Resolution: 120, 160, 240, 320, 0 = disabled)
uniform float u_PS1VertexSnapping;

out vec3 vWorldPos;
out vec3 vNormal;
smooth out vec2 vUV;
noperspective out vec2 vAffineUV;
out vec4 vFragPosLightSpace;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vFragPosLightSpace = lightSpaceMatrix * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vNormal = normalize(normalMatrix * aNormal);

    // Standard perspective UV vs Hardware PS1 Affine UV (no perspective divide)
    vUV = aUV;
    vAffineUV = aUV;

    vec4 clipPos = projection * view * worldPos;

    // PS1 GTE Fixed-Point Screen Coordinate Snapping (Vertex Jitter / Wobble)
    if (u_PS1VertexSnapping > 0.0)
    {
        vec2 ndc = clipPos.xy / clipPos.w;
        ndc = floor(ndc * u_PS1VertexSnapping + 0.5) / u_PS1VertexSnapping;
        clipPos.xy = ndc * clipPos.w;
    }

    gl_Position = clipPos;
}

