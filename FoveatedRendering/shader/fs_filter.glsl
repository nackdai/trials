#version 450
precision highp float;
precision highp int;

uniform vec4 invScreen;

uniform sampler2D s0;

void main()
{
    vec2 uv = gl_FragCoord.xy * invScreen.xy;

    uv.y = 1 - uv.y;

    gl_FragColor = texture2D(s0, uv);
}
