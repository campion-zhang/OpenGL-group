#version 100

attribute vec3 inPos;
attribute vec3 inTexcoord;

varying vec3 tc;

uniform mat4 view_matrix;

void main(void)
{
    tc = mat3(view_matrix) * inTexcoord;
    gl_Position = vec4(inPos, 1.0);
}
