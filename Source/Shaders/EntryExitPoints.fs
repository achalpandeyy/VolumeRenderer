#version 330 core

layout (location = 0) out vec4 out_frag_color;

in vec3 color;

void main()
{
    out_frag_color = vec4(color, 1.f);
}