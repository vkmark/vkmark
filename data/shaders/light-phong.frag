#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 ModelViewProjectionMatrix;
    uniform mat4 NormalMatrix;
    uniform vec4 MaterialDiffuse;
    uniform mat4 ModelViewMatrix;
};

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec4 in_position;

layout(location = 0) out vec4 frag_color;

vec4
compute_color(vec4 light_position, vec4 diffuse_light_color)
{
    const vec4 lightAmbient = vec4(0.011, 0.011, 0.011, 1.0);
    const vec4 lightSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 matAmbient = vec4(0.2, 0.2, 0.2, 1.0);
    const vec4 matSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    const float matShininess = 250.0;
    vec3 eye_direction = normalize(-in_position.xyz);
    vec3 light_direction = normalize(light_position.xyz/light_position.w -
                                     in_position.xyz/in_position.w);
    vec3 normalized_normal = normalize(in_normal);
    vec3 reflection = reflect(-light_direction, normalized_normal);
    float specularTerm = pow(max(0.0, dot(reflection, eye_direction)), matShininess);
    float diffuseTerm = max(0.0, dot(normalized_normal, light_direction));
    vec4 specular = (lightSpecular * matSpecular);
    vec4 ambient = (lightAmbient * matAmbient);
    vec4 diffuse = (diffuse_light_color * MaterialDiffuse);
    vec4 result = (specular * specularTerm) + ambient + (diffuse * diffuseTerm);
    return result;
}

void main(void)
{
    const vec4 LightSourcePosition = vec4(20.0, -20.0, 10.0, 1.0);
    const vec4 LightSourceDiffuse = vec4(0.8, 0.8, 0.8, 1.0);

    frag_color = compute_color(LightSourcePosition, LightSourceDiffuse);
}
