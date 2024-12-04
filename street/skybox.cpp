#include "skybox.h"
#include <render/shader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/gl.h>
#include <iostream>


void Skybox::initialize() {
    GLfloat vertices[] = {
        // Back face (-Z)
        -1.0f, -1.0f, -1.0f, // Bottom-left
        1.0f, -1.0f, -1.0f, // Bottom-right
        1.0f, 1.0f, -1.0f, // Top-right
        -1.0f, 1.0f, -1.0f, // Top-left
        // Front face (+Z)
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        // Left face (-X)
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        // Right face (+X)
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        // Top face (+Y)
        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        // Bottom face (-Y)
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
    };

    GLfloat uv_buffer_data[48] = {
        // Front (+Z)
        0.25f, 0.333f,
        0.5f, 0.333f,
        0.5f, 0.666f,
        0.25f, 0.666f,
        // Back (-Z)
        0.5f, 0.333f,
        0.75f, 0.333f,
        0.75f, 0.666f,
        0.5f, 0.666f,
        // Left (-X)
        0.25f, 0.333f,
        0.5f, 0.333f,
        0.5f, 0.666f,
        0.25f, 0.666f,
        // Right (+X)
        0.5f, 0.333f,
        0.75f, 0.333f,
        0.75f, 0.666f,
        0.5f, 0.666f,
        // Top (+Y)
        0.25f, 0.0f,
        0.5f, 0.0f,
        0.5f, 0.333f,
        0.25f, 0.333f,
        // Bottom (-Y)
        0.25f, 0.666f,
        0.5f, 0.666f,
        0.5f, 1.0f,
        0.25f, 1.0f,
    };

    // Create and bind UV buffer
    glGenBuffers(1, &uvBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    glGenBuffers(1, &vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    programID = LoadShadersFromFile("../street/skybox.vert", "../street/skybox.frag");
    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    textureID = LoadCubemapTexture("../street/sky_debug.png");
    if (textureID == 0) {
        std::cout << "Failed to load skybox texture" << std::endl;
    }
    textureSamplerID = glGetUniformLocation(programID, "skybox");
}

void Skybox::render(glm::mat4 viewProjection) {
    glDepthFunc(GL_LEQUAL);
    glUseProgram(programID);

    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
    glm::mat4 mvp = viewProjection * model;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glUniform1i(textureSamplerID, 0);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDepthFunc(GL_LESS);
}


void Skybox::cleanup() {
    glDeleteBuffers(1, &vertexBufferID);
    glDeleteVertexArrays(1, &vertexArrayID);
    glDeleteTextures(1, &textureID);
    glDeleteProgram(programID);
}
