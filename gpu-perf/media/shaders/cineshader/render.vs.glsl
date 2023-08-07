#version 100

attribute vec2 inPos;

void main(void)
{
    gl_Position = vec4(inPos.xy, 0.0, 1.0);
}
