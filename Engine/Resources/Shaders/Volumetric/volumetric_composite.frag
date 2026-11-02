#version 330 core

/**
 * @file volumetric_composite.frag
 * @brief Hacimsel ışıklandırma tamponunu sahne framebuffer'ına eklemeli (Additive Blending) olarak harmanlar.
 */

in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D u_VolumetricTexture;

void main()
{
    vec3 vol = texture(u_VolumetricTexture, v_TexCoords).rgb;
    FragColor = vec4(vol, 1.0);
}

