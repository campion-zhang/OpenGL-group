#version 100
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

attribute vec3 position;
attribute vec3 inNormal;

varying vec3 fragNormal;
varying vec3 fragView;

void main(void)
{
    vec4 pos_vs = mv_matrix * vec4(position,1.0);

    fragNormal = mat3(mv_matrix) * inNormal;
    fragView = pos_vs.xyz;

    gl_Position = proj_matrix * pos_vs;
}
