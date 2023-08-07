#version 430 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
uniform float time;
uniform mat4 view;

out VS_OUT
{
    vec4 color;
} vs_out;

void main()
{
    gl_Position = view*position;
    vs_out.color = color;
}
