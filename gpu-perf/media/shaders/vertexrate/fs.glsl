#version 100

precision mediump float;
varying vec4 vertexColor;

void main()
{
    gl_FragColor = vertexColor;
}
