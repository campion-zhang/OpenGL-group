#version 100
precision mediump float;
uniform samplerCube tex_cubemap;

varying vec3 tc;

void main(void)
{
    gl_FragColor = textureCube(tex_cubemap, tc);
}
