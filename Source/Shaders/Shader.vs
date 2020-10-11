#version 330 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;

out vec3 color;

uniform mat4 pvm;

void main()
{
    gl_Position = pvm * vec4(in_pos, 1.f);
    color = in_color;
}