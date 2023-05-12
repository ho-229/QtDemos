#version 440
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 texCoord;

out vec2 v_texCoord;
void main(void)
{
    gl_Position = vec4(vertex, 0, 1.0f);
    v_texCoord = texCoord;
}
