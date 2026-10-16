#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in ivec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 128;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

layout (std140) uniform BoneMatricesBlock
{
    mat4 u_Bones[MAX_BONES];
};

void main()
{
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
    vec4 skinnedPos = vec4(0.0);

    if (totalWeight > 0.0001)
    {
        for (int i = 0; i < 4; i++)
        {
            int boneID = aBoneIDs[i];
            float weight = aWeights[i];

            if (boneID >= 0 && boneID < MAX_BONES && weight > 0.0)
            {
                skinnedPos += (u_Bones[boneID] * vec4(aPos, 1.0)) * weight;
            }
        }
    }
    else
    {
        skinnedPos = vec4(aPos, 1.0);
    }

    gl_Position = lightSpaceMatrix * model * skinnedPos;
}

