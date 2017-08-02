#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 Transform;
};

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(location = 0) out vec2 out_texcoord;

void main(void)
{
    gl_Position = Transform * vec4(in_position, 0.0, 1.0);

    out_texcoord = in_texcoord;
}
