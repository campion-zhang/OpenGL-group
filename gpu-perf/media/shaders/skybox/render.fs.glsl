#version 100
precision mediump float;

uniform samplerCube tex_cubemap;

varying vec3 vVaryingTexCoord;

void main(void)
{
    gl_FragColor = textureCube(tex_cubemap, -vVaryingTexCoord) * vec4(0.85, 0.85, 0.85, 1.0);
}
