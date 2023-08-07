#version 100
#define TOTAL_PRIM_CNT 5
#define CUBE_VERTEX_CNT 36
attribute vec4 vertexPosition;
attribute vec4 vertexID;

uniform int prime_vertexcnt;
uniform int prime_offset;

uniform mat4 MVP[TOTAL_PRIM_CNT];

void main()
{
    int prime_index = prime_offset / CUBE_VERTEX_CNT +(int(vertexID.x) - prime_offset) / prime_vertexcnt;
    gl_Position = MVP[prime_index] * vertexPosition;

}
