#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aSize;

out vec4 particleColor;

uniform mat4 vpMatrix;

void main() {
    particleColor = aColor;
    gl_Position = vpMatrix * vec4(aPos, 1.0);
    gl_PointSize = aSize;
}