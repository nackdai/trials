#version 330
precision highp float;
precision highp int;

varying float viewDepth;;
varying float pointSize;
varying vec4 varColor;

uniform vec4 mtxC2V[4];
uniform vec4 mtxV2C[4];
uniform vec4 screen;
uniform float farClip;

void main()
{
    vec3 n;

    // ç¿ïWílÇ [0, 1] Å® [-1, 1] Ç…ïœä∑Ç∑ÇÈ.
    n.xy = gl_PointCoord * 2.0 - 1.0;

    // îºåaÇPÇÃâ~ÇÃíÜÇ…Ç†ÇÈÇ©Ç«Ç§Ç©.
    n.z = 1.0 - dot(n.xy, n.xy);
    if (n.z < 0.0) {
        // åãâ Ç™ïâÇ»ÇÁâ~ÇÃäOÇ…Ç†ÇÈÇÃÇ≈ÉtÉâÉOÉÅÉìÉgÇéÃÇƒÇÈ.
        discard;
    }

    float depth = viewDepth * farClip;

    vec2 uv = gl_FragCoord.xy / screen.xy;

    vec4 pos = vec4(uv.x, uv.y, 0.0, 1.0);
    pos.xy = pos.xy * 2.0 - 1.0;  // [0, 1] -> [-1, 1]

    pos.xy *= depth;

    // Clip -> View.
    pos.x = dot(pos, mtxC2V[0]);
    pos.y = dot(pos, mtxC2V[1]);
    pos.z = dot(pos, mtxC2V[2]);
    pos.w = dot(pos, mtxC2V[3]);

    pos.xy /= pos.z;
    pos.z = depth;

#if 0
    float weight = 1.0f - dot(n.xy, n.xy);
    pos.z -= weight * pointSize;
    pos.z = max(1.0, pos.z);

    float depth = pos.z / farClip;
#endif

    pos.x = dot(pos, mtxV2C[0]);
    pos.y = dot(pos, mtxV2C[1]);
    pos.z = dot(pos, mtxV2C[2]);
    pos.w = dot(pos, mtxV2C[3]);

    pos /= pos.w;

    //gl_FragDepth = pos.z;

#if 0
    //gl_FragData[0].xyz = viewPosition.zzz / farClip;
    gl_FragData[0].xyz = vec3(depth, depth, depth);
    gl_FragData[0].a = 1.0f;
    //gl_FragColor = vec4(1.0);
#else
    gl_FragColor = vec4(uv.x, uv.y, 0.0, 1.0);
#endif
}
