#version 100
#define TOTAL_PRIM_CNT 5
#define CUBE_VERTEX_CNT 36

attribute vec4 vertexNormal;
attribute vec4 vertexPosition;
attribute vec4 vertexColor;
attribute vec4 vertexID;

varying vec4 vColor;
varying vec4 vNormal;
varying vec4 vDepthPosition;

uniform int prime_vertexcnt;
uniform int prime_offset;

uniform mat4 MVP[TOTAL_PRIM_CNT];
uniform mat4 depthMVP[TOTAL_PRIM_CNT];

void main()
{
    int prime_index = prime_offset / CUBE_VERTEX_CNT +(int(vertexID.x) - prime_offset) / prime_vertexcnt;
    gl_Position = MVP[prime_index] * vertexPosition;
    vDepthPosition = depthMVP[prime_index] * vertexPosition;
    vNormal = vertexNormal;
    vColor = vertexColor;
}
