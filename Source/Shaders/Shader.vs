#version 330 core

layout (location = 0) in vec4 in_pos;

uniform mat4 pvm;

void main()
{
    gl_Position = in_pos;
}