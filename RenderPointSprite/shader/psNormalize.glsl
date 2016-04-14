#version 450
precision highp float;
precision highp int;

uniform sampler2D depthMap;
uniform sampler2D image;

uniform vec4 invScreen;

void main()
{
#if 0
    float depth = texelFetch(depthMap, ivec2(gl_FragCoord.xy), 0).r;

    if (depth >= 1.0) {
        discard;
    }

    vec4 color = texelFetch(image, ivec2(gl_FragCoord.xy), 0);
    color /= color.w;

    gl_FragColor = vec4(vec3(color.xyz), 1.0);
#else
    vec2 uv = gl_FragCoord.xy * invScreen.xy;
    uv.y = 1.0 - uv.y;

    float depth = texture2D(depthMap, uv).r;

    if (depth >= 1.0) {
        discard;
    }

    vec4 color = texture2D(image, uv);
    color /= color.w;

    gl_FragColor = vec4(vec3(color.xyz), 1.0);
#endif
}
