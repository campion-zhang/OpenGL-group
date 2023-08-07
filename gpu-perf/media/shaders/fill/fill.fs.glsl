#version 100

precision mediump float;

uniform sampler2D tex;
varying vec2 TexCoord;
varying vec4 FrontColor;
varying float r,g,b;

void main()
{
    vec4 t = texture2D(tex, TexCoord.xy);

    vec4 color = t*FrontColor;
    color.r = (color.r + r)*0.5;
    color.r = (color.r + sin(TexCoord.x))*0.5;
    color.g = (color.g + g)*0.5;
    color.g = (color.g + cos(TexCoord.y))*0.5;
    color.b = (color.b + b)*0.5;
    color.a = 1.0;

    gl_FragColor = color;
}



