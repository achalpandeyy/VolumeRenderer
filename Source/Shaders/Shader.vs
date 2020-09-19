#version 330 core

layout (location = 0) in vec3 v_pos;

out vec3 view_ray_direction;

uniform mat4 pv;
uniform vec3 cam_pos;

void main()
{
    view_ray_direction = v_pos - cam_pos;
    gl_Position = pv * vec4(v_pos, 1.f);
}