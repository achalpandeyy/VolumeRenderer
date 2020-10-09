#version 330 core

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 in_color;

out vec3 view_ray_direction;
out vec3 v_color;

uniform mat4 pvm;
uniform vec3 cam_pos;

void main()
{
    view_ray_direction = v_pos - cam_pos;
    gl_Position = pvm * vec4(v_pos, 1.f);
    v_color = in_color;
}