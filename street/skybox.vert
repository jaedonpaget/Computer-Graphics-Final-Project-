#version 330 core

layout(location = 0) in vec3 vertexPosition_modelspace;

out vec3 TexCoords;

uniform mat4 MVP;

void main(){
    TexCoords = vertexPosition_modelspace;
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1.0);
}