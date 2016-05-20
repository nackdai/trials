#version 330
precision highp float;
precision highp int;

attribute vec4 position;
attribute vec4 color_0;

varying vec4 var_Color;

uniform vec4 mtxW2C[4];
uniform float size;
uniform float scale;

void main()
{
    vec4 pt = position;
    pt.xyz *= scale;

    gl_Position.x = dot(pt, mtxW2C[0]);
    gl_Position.y = dot(pt, mtxW2C[1]);
    gl_Position.z = dot(pt, mtxW2C[2]);
    gl_Position.w = dot(pt, mtxW2C[3]);
    
    var_Color = color_0;
 
    //gl_PointSize = size / gl_Position.w;
    gl_PointSize = size;
}