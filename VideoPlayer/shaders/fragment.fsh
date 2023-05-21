#version 440
in vec2 v_texCoord;
out vec4 fragColor;

layout (location = 0) uniform sampler2D texY;
layout (location = 1) uniform sampler2D texU;
layout (location = 2) uniform sampler2D texV;
layout (location = 3) uniform sampler2D texSubtitle;

layout (location = 4) uniform mat3 colorConversion;
layout (location = 5) uniform bool is10Bit;

void main(void)
{
    vec3 yuv;
    yuv.x = texture(texY, v_texCoord).x;
    yuv.y = texture(texU, v_texCoord).x;
    yuv.z = texture(texV, v_texCoord).x;

    if(is10Bit)
    {
        vec3 yuv_h;
        yuv_h.x = texture(texY, v_texCoord).a;
        yuv_h.y = texture(texU, v_texCoord).a;
        yuv_h.z = texture(texV, v_texCoord).a;
        yuv = (yuv * 255.0 + yuv_h * 255.0 * 256.0) / 1023.0;
    }

    yuv -= vec3(16. / 255., 128. / 255., 128. / 255.);

    vec3 rgb = colorConversion * yuv;
    vec4 subtitle = texture(texSubtitle, v_texCoord);
    fragColor = mix(vec4(rgb, 1), subtitle, subtitle.a);
}
