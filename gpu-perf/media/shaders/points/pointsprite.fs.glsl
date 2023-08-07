#version 100

precision mediump float;

uniform sampler2D tex_star;                                    
varying vec4 starColor;
                                                               
void main(void)                                                
{                                                              
    gl_FragColor = starColor * texture2D(tex_star,gl_PointCoord);
}                                                              
