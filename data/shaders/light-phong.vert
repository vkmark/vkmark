#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 ModelViewProjectionMatrix;
    uniform mat4 NormalMatrix;
    uniform vec4 MaterialDiffuse;
    uniform mat4 ModelViewMatrix;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_position;

void main(void)
{
    vec4 current_position = vec4(in_position, 1.0);

    // Transform the normal to eye coordinates
    out_normal = normalize(vec3(NormalMatrix * vec4(in_normal, 1.0)));

    // Transform the current position to eye coordinates
    out_position = ModelViewMatrix * current_position;

    // Transform the current position to clip coordinates
    gl_Position = ModelViewProjectionMatrix * current_position;
}
