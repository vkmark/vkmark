#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 ModelViewProjectionMatrix;
    uniform mat4 NormalMatrix;
    uniform vec4 MaterialDiffuse;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 out_normal;

void main(void)
{
    // Transform the normal to eye coordinates
    out_normal = normalize(vec3(NormalMatrix * vec4(in_normal, 1.0)));

    // Transform the position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * vec4(in_position, 1.0);
}
