#version 420 core

layout(std140, binding = 0) uniform block {
    float TextureStepX;
    float TextureStepY;
};

layout(binding = 1) uniform sampler2D Texture0;

layout(location = 0) in vec2 in_texcoord;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    frag_color = texture(Texture0, in_texcoord);
}

