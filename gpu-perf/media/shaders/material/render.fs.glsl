#version 100

precision mediump float;

uniform sampler2D tex_envmap;

varying vec3 fragNormal;
varying vec3 fragView;

void main(void)
{
    vec3 normal = normalize(fragNormal);
    // u will be our normalized view vector
    vec3 u = normalize(fragView);

    // Reflect u about the plane defined by the normal at the fragment
    vec3 r = reflect(u, normalize(fragNormal));

    // Compute scale factor
    r.z += 1.0;
    float m = 0.5 * inversesqrt(dot(r, r));

    // Sample from scaled and biased texture coordinate
    gl_FragColor = texture2D(tex_envmap, r.xy * m + vec2(0.5));
    //gl_FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
