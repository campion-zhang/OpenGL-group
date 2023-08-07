#version 430 core

void main(void)
{
    const vec2 pos[] = vec2[](vec2(-1.0, -1.0),
                              vec2(1.0, -1.0),
                              vec2(-1.0, 1.0),
                              vec2(1.0, 1.0));

    vec2 pos_out = pos[gl_VertexID] + vec2(gl_InstanceID * 2.0, 0.0);

    gl_Position = vec4(pos_out.xy, 0.0, 1.0);

}
