#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 modelviewMatrix;
    uniform mat4 modelviewprojectionMatrix;
    uniform mat4 normalMatrix;
};

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec3 in_normal;

vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);

layout(location = 0) out vec4 vVaryingColor;

void main()
{
    gl_Position = modelviewprojectionMatrix * in_position;
    vec3 vEyeNormal = vec3(normalMatrix * vec4(in_normal, 1.0));
    vec4 vPosition4 = modelviewMatrix * in_position;
    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;
    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);
    float diff = max(0.0, dot(vEyeNormal, vLightDir));
    vVaryingColor = vec4(diff * in_color.rgb, 1.0);
}
