#version 100
attribute vec3 position;
uniform mat4 mvp;

varying vec3 pos;
varying vec4 glassColor;
varying float glassLinearDepth;
varying float w;

void main(void)
{
    vec4 color;
    float value = (position.x*position.x + position.y*position.y)*2.0;
    color.a = float(1)-value;
    color.r = 0.0;
    color.g = 0.8;
    color.b = 0.6;
    glassColor = color;
    gl_Position = mvp * vec4(position,1.0);
    w = gl_Position.w;
    pos = position;
}
