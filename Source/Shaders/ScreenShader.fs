#version 330 core

layout (location = 0) out vec4 color;

in vec2 tex_coords;

uniform sampler2D screen_texture;

void main()
{
	color = texture(screen_texture, tex_coords);
}