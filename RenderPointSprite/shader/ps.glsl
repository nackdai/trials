#version 450
precision highp float;
precision highp int;

varying vec4 viewSpace;
varying float pointSize;
varying vec4 varColor;

uniform vec4 mtxC2V[4];
uniform vec4 mtxV2C[4];
uniform vec4 mtxW2C[4];
uniform vec4 screen;
//uniform float farClip;

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

    vec4 pos = viewSpace;

    float weight = 1.0 - dot(n.xy, n.xy);
    //pos.z -= pointSize * weight;

    //pos.z /= farClip;

#if 1
    vec4 clipSpace;
    clipSpace.x = dot(viewSpace, mtxW2C[0]);
    clipSpace.y = dot(viewSpace, mtxW2C[1]);
    clipSpace.z = dot(viewSpace, mtxW2C[2]);
    clipSpace.w = dot(viewSpace, mtxW2C[3]);

    clipSpace /= clipSpace.w;
    clipSpace.xy = (clipSpace.xy + 1.0) * 0.5;
#else
    pos.xy = gl_FragCoord.xy / screen.xy;
    pos.xy = pos.xy * 2.0 - 1.0;  // [0, 1] -> [-1, 1]
    pos.y = -pos.y; // Invert.

    pos.z = gl_FragCoord.z;

    // NOTE
    // gl_FragCoord.w = 1 / clip.w
    // xyz /= gl_FragCoord.w => xyz *= clip.w
    pos.xyz /= gl_FragCoord.w;
    pos.w = 1 / gl_FragCoord.w;

    vec4 viewSpace;
    viewSpace.x = dot(pos, mtxC2V[0]);
    viewSpace.y = dot(pos, mtxC2V[1]);
    viewSpace.z = dot(pos, mtxC2V[2]);
    viewSpace.w = dot(pos, mtxC2V[3]);

    vec4 clipSpace;
    clipSpace.x = dot(viewSpace, mtxV2C[0]);
    clipSpace.y = dot(viewSpace, mtxV2C[1]);
    clipSpace.z = dot(viewSpace, mtxV2C[2]);
    clipSpace.w = dot(viewSpace, mtxV2C[3]);

    clipSpace /= clipSpace.w;
    clipSpace.xy = (clipSpace.xy + 1.0) * 0.5;
#endif

#if 0
    gl_FragData[0] = vec4(depth, depth, depth, 1.0);
#else
    gl_FragColor = vec4(vec3(clipSpace.z), 1.0);
#endif
}
