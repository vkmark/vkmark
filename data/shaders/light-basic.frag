#version 420 core

layout(location = 0) in vec4 in_color;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    frag_color = in_color;
}
