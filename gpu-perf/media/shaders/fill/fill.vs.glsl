#version 100
attribute vec4 position;
attribute vec2 coord;
attribute vec4 color;

uniform float time;
uniform float size;
varying vec2 TexCoord;
varying vec4 FrontColor;
varying float r,g,b;

void main()
{
    gl_Position = position;
    TexCoord = coord;
    FrontColor = color;

    r = (1.0+sin(0.9*time+0.2))*0.5;
    b = (1.0+sin(1.2*time+0.4))*0.5;
    g = (1.0+sin(1.5*time+0.6))*0.5;
}



