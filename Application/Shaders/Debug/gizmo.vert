#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 VertexColor;

uniform mat4 u_ViewProj;

void main()
{
    VertexColor = aColor;
    gl_Position = u_ViewProj * vec4(aPos, 1.0f);
}