#version 330 core

out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;

uniform float time;
uniform vec3 lightPos;

void main()
{
    vec3 L = normalize(lightPos - vWorldPos);
    vec3 N = normalize(vNormal);

    float diff = max(dot(N, L), 0.0);
    float ambient = 0.25;

    float light = ambient + diff * 0.75;

    float levels = 6.0;
    light = floor(light * levels) / levels;

    vec3 color = vec3(light);

    mat4 bayer = mat4(
         0.0,  8.0,  2.0, 10.0,
        12.0,  4.0, 14.0,  6.0,
         3.0, 11.0,  1.0,  9.0,
        15.0,  7.0, 13.0,  5.0
    ) / 16.0;

    ivec2 p = ivec2(gl_FragCoord.xy) & 3;
    float threshold = bayer[p.x][p.y];

    color += (threshold - 0.5) * 0.08;

    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}