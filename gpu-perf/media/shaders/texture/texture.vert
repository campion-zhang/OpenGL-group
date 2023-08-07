#version 100

attribute vec4 av4position;
attribute vec2 in_tex_coord;

uniform mat4 mvp;

varying vec2 vs_tex_coord;

void main()
{
    vs_tex_coord = in_tex_coord;
    gl_Position = mvp * av4position;
}

