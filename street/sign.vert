// sign.vert
#version 330 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 vertexNormal;

uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

out vec2 UV;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 Normal_worldspace;

void main() {
    gl_Position = MVP * vec4(vertexPosition, 1);

    // Position of the vertex in worldspace
    Position_worldspace = (modelMatrix * vec4(vertexPosition, 1)).xyz;

    // Normal in camera space and world space
    Normal_worldspace = normalMatrix * vertexNormal;
    Normal_cameraspace = Normal_worldspace;

    // UV coordinates
    UV = vertexUV;
}
