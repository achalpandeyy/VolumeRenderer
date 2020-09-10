#version 460 core

// layout (location = 0) out vec4 frag_color;
out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D texture_sampler;

void main()
{
    frag_color = texture(texture_sampler, tex_coords);
}