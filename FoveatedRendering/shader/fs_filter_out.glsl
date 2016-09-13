#version 450
precision highp float;
precision highp int;

uniform vec4 invScreen;

uniform sampler2D s0;
uniform sampler2D s1;

#define SCREEN_HEIGHT 719

uniform bool isFilter;

void main()
{
    vec2 texel = invScreen.xy;
    
    ivec2 xy = ivec2(gl_FragCoord.x, gl_FragCoord.y);
    xy.y = SCREEN_HEIGHT - xy.y;

    vec2 uv = gl_FragCoord.xy * invScreen.xy;

    vec4 mask = texture2D(s1, uv);

    if (isFilter) {
        if (mask.r > 0) {
            gl_FragColor = texture2D(s0, uv);
        }
        else {
            // NOTE
            // gl_FragCoord は lower-left 基点.

#if 1
            ivec2 base = ivec2(0, 0);
            ivec2 offset = ivec2(0, 0);

            int xp = xy.x % 3;
            int yp = xy.y % 3;

            if (yp == 0) {
                if (xp == 0) {
                    offset.x -= 2;
                    offset.y += 2;
                    base.x += 1;
                    base.y -= 1;
                }
                else if (xp == 1) {
                    offset.y += 2;
                    base.y -= 1;
                }
                else {
                    offset.x += 2;
                    offset.y += 2;
                    base.x -= 1;
                    base.y -= 1;
                }
            }
            else if (yp == 1) {
                if (xp == 0) {
                    offset.x -= 2;
                    base.x += 1;
                }
                else if (xp == 2) {
                    offset.x += 2;
                    base.x -= 1;
                }
            }
            else {
                if (xp == 0) {
                    offset.x -= 2;
                    offset.y -= 2;
                    base.x += 1;
                    base.y += 1;
                }
                else if (xp == 1) {
                    offset.y -= 2;
                    base.y += 1;
                }
                else {
                    offset.x += 2;
                    offset.y -= 2;
                    base.x -= 1;
                    base.y += 1;
                }
            }

            base += ivec2(gl_FragCoord.x, gl_FragCoord.y);
            offset += ivec2(gl_FragCoord.x, gl_FragCoord.y);

            // 中心色.
            uv = vec2(base.x + 0.5, base.y + 0.5) * invScreen.xy;
            vec4 baseclr = texture2D(s0, uv);

            // 周辺色.
            uv = vec2(offset.x + 0.5, offset.y + 0.5) * invScreen.xy;
            gl_FragColor = texture2D(s0, uv);

            gl_FragColor = (2 * baseclr + gl_FragColor) / 3;
            gl_FragColor.a = 1;
#else
            gl_FragColor = texture2D(s0, uv);
#endif
        }
    }
    else {
        gl_FragColor = texture2D(s0, uv);
    }
}
