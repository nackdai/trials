#version 450
precision highp float;
precision highp int;

attribute vec4 position;
attribute vec4 color_0;

varying vec4 viewSpace;
varying float pointSize;
varying vec4 varColor;

uniform vec4 mtxW2C[4];
uniform float size;

void main()
{
    viewSpace = position;

    gl_Position.x = dot(position, mtxW2C[0]);
    gl_Position.y = dot(position, mtxW2C[1]);
    gl_Position.z = dot(position, mtxW2C[2]);
    gl_Position.w = dot(position, mtxW2C[3]);
     
    gl_PointSize = size / gl_Position.w;//720.0 * mtxV2C[1].y * size / gl_Position.w;
    pointSize = gl_PointSize;

    varColor = color_0;
}