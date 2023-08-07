#version 100 core
#define TOTAL_PRIM_CNT 164
#define CUBE_VERTEX_CNT 36

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexNormal;
layout(location = 2) in vec4 vertexColor;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec4 vNormal;
layout(location = 2) out vec4 vDepthPosition;

uniform int prime_vertexcnt;
uniform int prime_offset;

layout(binding = 1) uniform rTransform {
    mat4 MVP[TOTAL_PRIM_CNT];
    mat4 depthMVP[TOTAL_PRIM_CNT];
};

layout (location = 1) uniform vec4 Plane;

void main()
{
    uint prime_index = prime_offset / CUBE_VERTEX_CNT +(gl_VertexID - prime_offset) / prime_vertexcnt;
    gl_Position = MVP[prime_index] * vertexPosition;
    vDepthPosition = depthMVP[prime_index] * vertexPosition;
    vNormal = vertexNormal;
    vColor = vertexColor;
    gl_CullDistance[0] = Plane[3] - Plane[0]*vertexPosition.x - Plane[1]*vertexPosition.y - Plane[2]*vertexPosition.z;
}
