#version 420 core

layout(binding = 1) uniform sampler2D MaterialTexture0;

layout(location = 0) in vec2 in_texcoord;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    vec4 texel = texture(MaterialTexture0, in_texcoord);
    frag_color = texel;
}
