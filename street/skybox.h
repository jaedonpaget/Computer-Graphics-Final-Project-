#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

class Skybox {
public:
    void initialize();
    void render(glm::mat4 viewProjection);
    void cleanup();


private:
    GLuint vertexArrayID;
    GLuint uvBufferID;
    GLuint vertexBufferID;
    GLuint textureID;
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
};