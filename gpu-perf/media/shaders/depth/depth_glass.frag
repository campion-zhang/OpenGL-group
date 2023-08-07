#version 100
precision mediump float;
uniform float near;
uniform float far;
uniform float glassMaterial;

uniform vec2 viewportSize;
uniform sampler2D cubeDepthTexture;

varying float w;
varying vec4 glassColor;
varying vec3 pos;

float getValue(float z,float n,float f)
{
    return 2.0  / (f + n - z * (f - n));
}

void main(void)
{
    float z = texture2D(cubeDepthTexture, gl_FragCoord.xy / viewportSize).r * 2.0 - 1.0;
    float fCubeLinearDepth = getValue(z,near,far);
    
    vec4 color = glassColor;
    color.a = 3.2*(pos.x*pos.x+pos.y*pos.y);

    float factor = clamp(color.a + (fCubeLinearDepth - w/(far-near)) * glassMaterial, 0.0, 1.0);
    gl_FragColor = vec4(color.rgb,factor);
}

