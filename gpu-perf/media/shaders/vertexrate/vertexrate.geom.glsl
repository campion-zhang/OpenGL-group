#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
uniform float time;

in VS_OUT
{
    vec4 color;
} gs_in[];

out GS_OUT
{
    vec4 color;
} gs_out;

void main()
{
    vec3 center = vec3(0,0,0);
    for(int i=0;i<gs_in.length();i++)
    {
        gl_Position = gl_in[i].gl_Position;
        center += gl_Position.xyz;
    }

    center /= 3.0;

    float scale1 = 0.5*(cos(time*2.4)+1.0);
    float scale2 = 0.5*(sin(time*2.4)+1.0);
    float scale3 = 0.5*(cos(time*3.6)+1.0);

    float size = (gl_in[1].gl_Position.x-gl_in[0].gl_Position.x)*scale1;

    gs_out.color = gs_in[0].color;
    gl_Position = gl_in[0].gl_Position+vec4(-size*scale1,-size*scale2,0,0);
    EmitVertex();

    gs_out.color = gs_in[1].color;

    gl_Position = gl_in[1].gl_Position+vec4(size*scale2,-size*scale3,0,0);
    EmitVertex();

    gs_out.color = gs_in[2].color;
    gl_Position = gl_in[2].gl_Position+vec4(-size*scale3,size*scale1,0,0);
    EmitVertex();

    EndPrimitive();
}

