#version 330
precision highp float;
precision highp int;

attribute vec3 position;
attribute vec4 color_0;

varying vec4 var_Color;

uniform vec4 mtxW2C[4];
uniform float size;

void main()
{
    vec4 pos = vec4(position, 1.0);

    gl_Position.x = dot(pos, mtxW2C[0]);
    gl_Position.y = dot(pos, mtxW2C[1]);
    gl_Position.z = dot(pos, mtxW2C[2]);
    gl_Position.w = dot(pos, mtxW2C[3]);
    
    var_Color = color_0;
 
    gl_PointSize = size / gl_Position.w;
}