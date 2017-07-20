#version 420 core

layout(std140, binding = 0) uniform block {
    uniform mat4 ModelViewProjectionMatrix;
    uniform mat4 NormalMatrix;
    uniform vec4 MaterialDiffuse;
};

layout(location = 0) in vec3 in_normal;

layout(location = 0) out vec4 frag_color;

void main(void)
{
    const vec4 LightSourceAmbient = vec4(0.011, 0.011, 0.011, 1.0);
    const vec4 LightSourceDiffuse = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 LightSourceSpecular = vec4(0.8, 0.8, 0.8, 1.0);
    const vec4 MaterialAmbient = vec4(1.0, 1.0, 1.0, 1.0);
    const vec4 MaterialSpecular = vec4(1.0, 1.0, 1.0, 1.0);
    const float MaterialShininess = 250.0;
    const vec4 LightSourcePosition = vec4(20.0, -20.0, 10.0, 1.0);
    const vec3 LightSourceHalfVector = vec3(0.40824, -0.40824, 0.81649);

    vec3 N = normalize(in_normal);

    // In the lighting model we are using here (Blinn-Phong with light at
    // infinity, viewer at infinity), the light position/direction and the
    // half vector is constant for the all the fragments.
    vec3 L = normalize(LightSourcePosition.xyz);
    vec3 H = normalize(LightSourceHalfVector);

    // Calculate the diffuse color according to Lambertian reflectance
    vec4 diffuse = MaterialDiffuse * LightSourceDiffuse * max(dot(N, L), 0.0);

    // Calculate the ambient color
    vec4 ambient = MaterialAmbient * LightSourceAmbient;

    // Calculate the specular color according to the Blinn-Phong model
    vec4 specular = MaterialSpecular * LightSourceSpecular *
                    pow(max(dot(N,H), 0.0), MaterialShininess);

    // Calculate the final color
    frag_color = vec4((ambient + specular + diffuse).xyz, 1.0);
}
