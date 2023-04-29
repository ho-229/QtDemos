﻿varying vec2 v_texCoord;

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

uniform int pixFmt;

void main(void)
{
    vec3 yuv;
    yuv.x = texture2D(tex_y, v_texCoord).r;
    yuv.y = texture2D(tex_u, v_texCoord).r - 0.5;
    yuv.z = texture2D(tex_v, v_texCoord).r - 0.5;

    vec3 rgb;
    if (pixFmt == 0) {
        // YUV420p
        rgb = mat3(1.0,    1.0,     1.0,
                   0.0,    -0.3455, 1.779,
                   1.4075, -0.7169, 0.0) * yuv;
    } else {
        // YUV444P
        rgb.x = clamp(yuv.x + 1.402 * yuv.z, 0.0, 1.0);
        rgb.y = clamp(yuv.x - 0.34414 * yuv.y - 0.71414 * yuv.z, 0.0, 1.0);
        rgb.z = clamp(yuv.x + 1.772 * yuv.y, 0.0, 1.0);
    }

    gl_FragColor = vec4(rgb, 1.0);
}

