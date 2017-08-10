#version 420 core

layout(std140, binding = 0) uniform block {
    float TextureStepX;
    float TextureStepY;
};

layout(binding = 1) uniform sampler2D Texture0;

layout(location = 0) in vec2 in_texcoord;

layout(location = 0) out vec4 frag_color;

const float Kernel0 = 0.000000;
const float Kernel1 = 0.250000;
const float Kernel2 = 0.000000;
const float Kernel3 = 0.250000;
const float Kernel4 = -1.000000;
const float Kernel5 = 0.250000;
const float Kernel6 = 0.000000;
const float Kernel7 = 0.250000;
const float Kernel8 = 0.000000;

void main(void)
{
    vec4 result;

    result =
        texture(Texture0, in_texcoord + vec2(-1.0 * TextureStepX, -1.0 * TextureStepY)) * Kernel0 +
        texture(Texture0, in_texcoord + vec2(0.0 * TextureStepX, -1.0 * TextureStepY)) * Kernel1 +
        texture(Texture0, in_texcoord + vec2(1.0 * TextureStepX, -1.0 * TextureStepY)) * Kernel2 +
        texture(Texture0, in_texcoord + vec2(-1.0 * TextureStepX, 0.0 * TextureStepY)) * Kernel3 +
        texture(Texture0, in_texcoord + vec2(0.0 * TextureStepX, 0.0 * TextureStepY)) * Kernel4 +
        texture(Texture0, in_texcoord + vec2(1.0 * TextureStepX, 0.0 * TextureStepY)) * Kernel5 +
        texture(Texture0, in_texcoord + vec2(-1.0 * TextureStepX, 1.0 * TextureStepY)) * Kernel6 +
        texture(Texture0, in_texcoord + vec2(0.0 * TextureStepX, 1.0 * TextureStepY)) * Kernel7 +
        texture(Texture0, in_texcoord + vec2(1.0 * TextureStepX, 1.0 * TextureStepY)) * Kernel8;

    frag_color = vec4(result.xyz, 1.0);
}
