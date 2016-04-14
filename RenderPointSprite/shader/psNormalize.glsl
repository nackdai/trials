#version 450
precision highp float;
precision highp int;

uniform sampler2D depthMap;
uniform sampler2D image;

void main()
{
#if 1
    float depth = texelFetch(depthMap, ivec2(gl_FragCoord.xy), 0).r;

    if (depth >= 1.0) {
        discard;
    }

    vec4 color = texelFetch(image, ivec2(gl_FragCoord.xy), 0);
    color /= color.w;

    gl_FragColor = vec4(vec3(color.xyz), 1.0);

    //gl_FragDepth = depth;
#else
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
#endif
}
