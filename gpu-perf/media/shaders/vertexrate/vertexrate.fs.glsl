#version 330
uniform float time;

in GS_OUT
{
    vec4 color;
} fs_in;

in vec4 vs_color;

void main()
{
    gl_FragColor = fs_in.color;
}



