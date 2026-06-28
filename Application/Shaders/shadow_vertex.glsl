#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    // Vertex pozisyonunu doğrudan ışığın gözünden dünya ve projeksiyon matrisleriyle çarpıyoruz
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}