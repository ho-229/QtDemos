#version 440
in vec2 v_texCoord;
out vec4 fragColor;

layout (location = 0) uniform sampler2D texY;
layout (location = 1) uniform sampler2D texU;
layout (location = 2) uniform sampler2D texV;

layout (location = 3) uniform bool isYuv420;

void main(void)
{
    vec3 yuv;
    yuv.x = texture(texY, v_texCoord).x;
    yuv.y = texture(texU, v_texCoord).x - 0.5;
    yuv.z = texture(texV, v_texCoord).x - 0.5;

    vec3 rgb;
    if (isYuv420) {
        // YUV420P
        rgb = mat3(1.0,    1.0,     1.0,
                   0.0,    -0.3455, 1.779,
                   1.4075, -0.7169, 0.0) * yuv;
    } else {
        // YUV444P
        rgb.x = clamp(yuv.x + 1.402 * yuv.z, 0.0, 1.0);
        rgb.y = clamp(yuv.x - 0.34414 * yuv.y - 0.71414 * yuv.z, 0.0, 1.0);
        rgb.z = clamp(yuv.x + 1.772 * yuv.y, 0.0, 1.0);
    }

    fragColor = vec4(rgb, 1);
}
