#version 100
precision mediump float;

varying vec3 vv3color;

void main(void)
{
    gl_FragColor = vec4(vv3color, 1.0);
}
