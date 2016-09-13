#version 450
precision highp float;
precision highp int;

uniform vec4 invScreen;
uniform vec4 center;

#define RADIUS  180
#define SCREEN_HEIGHT 719
#define IPD     6

#define ellipseA (RADIUS + IPD * 0.5)
#define ellipseB RADIUS

#define AREA_W_0_HALF    (480)
#define AREA_H_0_HALF    (270)

#if 1
bool isInCircle(vec2 xy)
{
    vec2 tmp = gl_FragCoord.xy - center.xy;
    float dist = dot(tmp, tmp);

    return (dist <= RADIUS* RADIUS);
}
#else
bool isInCircle(vec2 xy)
{
    vec2 tmp = gl_FragCoord.xy - center.xy;
    float d = tmp.x * tmp.x / (ellipseA * ellipseA) + tmp.y * tmp.y / (ellipseB * ellipseB);

    return (d <= 1);
}
#endif

void main()
{
    ivec2 xy = ivec2(gl_FragCoord.x, gl_FragCoord.y);
    xy.y = SCREEN_HEIGHT - xy.y;

    vec2 tmp = abs(gl_FragCoord.xy - center.xy);

    if (isInCircle(gl_FragCoord.xy)) {
        // フィルタリングしないエリア.
        gl_FragColor = vec4(1, 1, 0, 1);
    }
    else if (tmp.x < AREA_W_0_HALF + 3 && tmp.y < AREA_H_0_HALF + 3) {
        if (tmp.x < AREA_W_0_HALF && tmp.y < AREA_H_0_HALF) {
            int xp = xy.x & 0x03;
            int yp = xy.y & 0x03;

            if (yp <= 1) {
                if (xp > 1) {
                    gl_FragColor = vec4(1, 0, 0, 1);
                }
                else {
                    gl_FragColor = vec4(0, 0, 0, 1);
                }
            }
            else {
                if (xp <= 1) {
                    gl_FragColor = vec4(1, 0, 0, 1);
                }
                else {
                    gl_FragColor = vec4(0, 0, 0, 1);
                }
            }
        }
        else {
            // フィルタリングしないエリア.
            // 内周と外周の間のバッファリングエリア.
            gl_FragColor = vec4(1, 1, 0, 1);
        }
    }
    else {
        int xp = xy.x % 3;
        int yp = xy.y % 3;

        if (yp == 1) {
            if (xp == 1) {
                gl_FragColor = vec4(1, 0, 0, 1);
            }
            else {
                gl_FragColor = vec4(0, 0, 0, 1);
            }
        }
        else {
            gl_FragColor = vec4(0, 0, 0, 1);
        }
    }
}
