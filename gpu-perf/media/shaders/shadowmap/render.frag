#version 100
precision mediump float;
varying vec4 vColor;
varying vec4 vNormal;
varying vec4 vDepthPosition;

uniform sampler2D shadowMap;
const float bias = 0.005;
float PCF()
{
    float shadowComponent = 0.0;
    vec2 texelSize = vec2(0.5 / 600.0, 0.5 / 600.0);

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float currentSampleDepth = texture2D(shadowMap, vDepthPosition.xy + vec2(x, y) * texelSize).r;
            shadowComponent += (vDepthPosition.z - bias > currentSampleDepth) ? 0.0 : 1.0;
        }
    }

    return shadowComponent /= 25.0;
}

void main()
{
    float visibility = mix(1.0, PCF(), 0.5);
    gl_FragColor = vec4(visibility * vColor.rgb, vColor.a);
}
