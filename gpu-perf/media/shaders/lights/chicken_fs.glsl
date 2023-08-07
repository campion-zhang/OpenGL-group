#version 100
precision mediump float;
varying vec2 texCoord;
varying vec3 fragNormal;
varying vec3 fragDiffuse;
varying vec3 fragSpecular;
varying vec3 fragAmbient;
varying vec2 fragTc;
varying vec3 fragWorldPos;
varying vec3 fragLightPos;

varying float fragDiffStrength;
varying float fragSpecStrength;
varying float fragAmbiStrength;

varying vec3 fragLightDir;
varying vec3 fragLightColor;
varying vec3 fragViewDir;

uniform sampler2D texture01;
uniform sampler2D texture02;

uniform int useCartoon;

void main()
{
    vec3 objColor = vec3(0.0,1.0,0.819);
    if (useCartoon == 0) {
        vec3 finalLight = vec3(texture2D(texture02, fragTc));
        gl_FragColor = vec4(finalLight * objColor * 0.9, 1.0);
    } else if (useCartoon == 1) {
        vec3 finalLight = fragDiffuse + fragAmbient;
        gl_FragColor = vec4(finalLight * objColor, 1.0 );

    }  else if (useCartoon == 2) {
        vec3 finalLight = fragDiffuse + fragSpecular + fragAmbient;
        gl_FragColor = vec4(finalLight * objColor, 1.0 );

    } else if (useCartoon == 3) {
        vec3 lightDir = normalize(fragLightPos - fragWorldPos);
        vec3 diffuse = max( dot(lightDir, fragNormal), 0.0 ) * fragLightColor;

        vec3 reflectVec = normalize( reflect( -lightDir, fragNormal) );
        float spec = max( dot(reflectVec,  fragViewDir), 0.0 );
        vec3 specular = pow(spec, 128.0) * fragLightColor;

        vec3 finalLight = (diffuse * fragDiffStrength + specular * fragSpecStrength + fragLightColor * fragAmbiStrength);
        gl_FragColor = vec4(finalLight * objColor, 1.0 );
    } else {
        vec3 lightDir = normalize(fragLightPos - fragWorldPos);
        vec3 diffuse = max( dot(lightDir, fragNormal), 0.0 ) * fragLightColor;

        vec3 reflectVec = normalize( reflect( -lightDir, fragNormal) );
        float spec = max( dot(fragNormal,  normalize(lightDir + fragViewDir)), 0.0 );
        vec3 specular = pow(spec, 128.0) * fragLightColor;

        vec3 finalLight = (diffuse * fragDiffStrength + specular*fragSpecStrength + fragLightColor * fragAmbiStrength);
        gl_FragColor = vec4(finalLight * objColor, 1.0 );
    }

}
