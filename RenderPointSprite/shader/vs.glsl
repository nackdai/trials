#version 330
precision highp float;
precision highp int;

attribute vec4 position;
attribute vec4 color_0;

varying float viewDepth;
varying float pointSize;
varying vec4 varColor;

uniform vec4 mtxW2V[4];
uniform vec4 mtxV2C[4];
uniform float farClip;
uniform float size;

void main()
{
    vec4 viewSpace;
    viewSpace.x = dot(position, mtxW2V[0]);
    viewSpace.y = dot(position, mtxW2V[1]);
    viewSpace.z = dot(position, mtxW2V[2]);
    viewSpace.w = dot(position, mtxW2V[3]);
    viewDepth = viewSpace.z / farClip;

    gl_Position.x = dot(viewSpace, mtxV2C[0]);
    gl_Position.y = dot(viewSpace, mtxV2C[1]);
    gl_Position.z = dot(viewSpace, mtxV2C[2]);
    gl_Position.w = dot(viewSpace, mtxV2C[3]);
     
    gl_PointSize = 720.0 * mtxV2C[1].y * size / gl_Position.w;
    pointSize = gl_PointSize;

    varColor = color_0;
}