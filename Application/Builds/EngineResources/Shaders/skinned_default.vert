#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 128;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

layout (std140) uniform BoneMatricesBlock
{
    mat4 u_Bones[MAX_BONES];
};

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out mat3 vTBN;
out vec4 vFragPosLightSpace;

void main()
{
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;

    vec4 skinnedPos = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);
    vec3 skinnedTangent = vec3(0.0);

    if (totalWeight > 0.0001)
    {
        for (int i = 0; i < 4; i++)
        {
            int boneID = aBoneIDs[i];
            float weight = aWeights[i];

            if (boneID >= 0 && boneID < MAX_BONES && weight > 0.0)
            {
                mat4 boneMat = u_Bones[boneID];
                skinnedPos += (boneMat * vec4(aPos, 1.0)) * weight;

                mat3 normalMat = mat3(boneMat);
                skinnedNormal += (normalMat * aNormal) * weight;
                skinnedTangent += (normalMat * aTangent) * weight;
            }
        }
    }
    else
    {
        skinnedPos = vec4(aPos, 1.0);
        skinnedNormal = aNormal;
        skinnedTangent = aTangent;
    }

    vec4 worldPos = model * skinnedPos;
    vWorldPos = worldPos.xyz;
    vFragPosLightSpace = lightSpaceMatrix * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 N = normalize(normalMatrix * skinnedNormal);
    vec3 T = normalize(normalMatrix * skinnedTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt orthogonalize
    vec3 B = cross(N, T);

    vTBN = mat3(T, B, N);
    vNormal = N;
    vUV = aUV;

    gl_Position = projection * view * worldPos;
}

