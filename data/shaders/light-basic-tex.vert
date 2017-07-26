#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 ModelViewProjectionMatrix;
    uniform mat4 NormalMatrix;
    uniform vec4 MaterialDiffuse;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texcoord;

vec4 LightSourcePosition = vec4(20.0, -20.0, 10.0, 1.0);

void main(void)
{
    // Transform the normal to eye coordinates
    vec3 N = normalize(vec3(NormalMatrix * vec4(in_normal, 1.0)));

    // The LightSourcePosition is actually its direction for directional light
    vec3 L = normalize(LightSourcePosition.xyz);

    // Multiply the diffuse value by the vertex color (which is fixed in this case)
    // to get the actual color that we will use to draw this vertex with
    float diffuse = max(dot(N, L), 0.0);
    out_color = vec4(diffuse * MaterialDiffuse.rgb, MaterialDiffuse.a);

    out_texcoord = in_texcoord;

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(in_position, 1.0);
}
