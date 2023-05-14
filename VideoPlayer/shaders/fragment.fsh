#version 440
in vec2 v_texCoord;
out vec4 fragColor;

layout (location = 0) uniform sampler2D texY;
layout (location = 1) uniform sampler2D texU;
layout (location = 2) uniform sampler2D texV;
layout (location = 3) uniform sampler2D texSubtitle;

layout (location = 4) uniform mat3 colorConversion;

void main(void)
{
    vec3 yuv;
    yuv.x = texture(texY, v_texCoord).x - 16. / 255.;
    yuv.y = texture(texU, v_texCoord).x - 128. / 255.;
    yuv.z = texture(texV, v_texCoord).x - 128. / 255.;

    vec3 rgb = colorConversion * yuv;
    vec4 subtitle = texture(texSubtitle, v_texCoord);
    fragColor = mix(vec4(rgb, 1), subtitle, subtitle.a);
}
