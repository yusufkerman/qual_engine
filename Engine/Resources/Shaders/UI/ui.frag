#version 330 core
in vec2 v_TexCoord;
in vec4 v_Color;

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform bool u_HasTexture;
uniform bool u_IsFontSDF;

void main()
{
    vec4 texColor = vec4(1.0);
    if (u_HasTexture)
    {
        if (u_IsFontSDF)
        {
            float alpha = texture(u_Texture, v_TexCoord).r;
            texColor = vec4(1.0, 1.0, 1.0, alpha);
        }
        else
        {
            texColor = texture(u_Texture, v_TexCoord);
        }
    }

    FragColor = texColor * v_Color;
}

