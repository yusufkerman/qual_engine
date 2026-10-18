#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 v_TexCoord;
out vec4 v_Color;

uniform mat4 u_Projection;

void main()
{
    v_TexCoord = aTexCoord;
    v_Color = aColor;
    gl_Position = u_Projection * vec4(aPos, 0.0, 1.0);
}

