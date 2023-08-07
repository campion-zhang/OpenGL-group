#version 100
                                                              
attribute vec4 position;
attribute vec4 color;
                                                              
uniform float time;                                           
uniform mat4 proj_matrix;                                     
uniform float angle;     
uniform float fstep;
                                                    
varying vec4 starColor;
                                                              
void main(void)                                               
{                                                             
    vec4 newVertex = position;                                
    float dgree = 0.0;
    float radio = newVertex.y / (newVertex.x + 0.000000001);
    float dgr = asin(radio);
    if(newVertex.x < 0.0 && newVertex.y > 0.0 || newVertex.x < 0.0 && newVertex.y < 0.0)
        dgree = dgr + 3.14159265;
    else
        dgree = dgr;

    float degree = degrees(dgree);
    newVertex.x += fstep;
    newVertex.y += fstep;
                                                                                         
    newVertex.z += time;
    newVertex.z = fract(newVertex.z);
                                                        
    float size = (30.0 * newVertex.z * newVertex.z);
                                                              
    starColor = smoothstep(6.0, 7.0, size) * color;   

    float dst = distance(newVertex.xy, vec2(0.0,0.0));
    newVertex.x = dst * sin(degree + angle) * sin(time*0.2);
    newVertex.y = dst * cos(degree + angle) * cos(time*0.2);
    newVertex.z = (999.99 * newVertex.z) - 1000.0;
    gl_Position = proj_matrix * newVertex;                    
    gl_PointSize = size;                                      
}                                                             
