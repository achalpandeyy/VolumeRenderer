#version 460 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coords;

out vec2 tex_coords;

uniform mat4 pvm;

void main()
{
    tex_coords = vertex_tex_coords;
    gl_Position = pvm * vec4(vertex_pos, 1.0);
}