#version 420 core

layout(location = 0) in vec4 vVaryingColor;
layout(location = 0) out vec4 f_color;

void main()
{
    f_color = vVaryingColor;
}
