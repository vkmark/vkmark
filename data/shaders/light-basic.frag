#version 420 core

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_texcoord;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    frag_color = in_color;
}
