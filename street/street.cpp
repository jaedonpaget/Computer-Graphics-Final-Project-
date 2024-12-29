#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iomanip>
#include <render/shader.h>

#include <algorithm>
#include <cmath>
#include <stb/stb_image.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#include "particle.h"
#include "sand.h"
#include "bot.h"
#include "Floor.h"
#include "lightInfo.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

static void mouse_callback(GLFWwindow *window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 eye_center;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);
static glm::vec3 cameraPosition(0.0f, 15.0f, 300.0f); // Start just above the floor level
static glm::vec3 cameraFront(0.0f, 0.0f, -1.0f); // Direction the camera is facing
static glm::vec3 cameraRight(1.0f, 0.0f, 0.0f); // Perpendicular to cameraFront
static glm::vec3 cameraUp(0.0f, 1.0f, 0.0f); // World up

// View control
static float viewAzimuth = 0.f;
static float viewPolar = 0.f;
static float viewDistance = 1000.0f;

// Variables for cursor view control
static double lastX = 400, lastY = 300;
static float yaw = -90.0f; //Facing negative z
static float pitch = 0.0f;
static bool firstMouse = true;

// Lighting Control
static glm::vec3 lightPosition(500.0f, 500.0f, 0.0f); // Sun's position
static glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // Warm light color
static float lightIntensity = 1.2f; // Brightness multiplier
static glm::vec3 lightLookAt(0.0f, 0.0f, 0.0f); // Where the light is pointing
static glm::vec3 lightDirection; // Computed in each frame
Light sunLightInfo(lightDirection, lightPosition, lightColor, lightLookAt, lightIntensity);

static float depthFoV = 90.0f;
static float depthNear = 90.0f;
static float depthFar = 1000.0f;

static float lightSpeed = 100.0f;

const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
GLuint depthMapFBO;
GLuint depthMap;

static bool playAnimation = true;
static float playbackSpeed = 2.0f;




static GLuint LoadTextureTileBox(const char *texture_file_path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(false);
    uint8_t *img = stbi_load(texture_file_path, &w, &h, &channels, 3);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // To tile textures on a box, we set wrapping to repeat
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (img) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << texture_file_path << std::endl;
    }
    stbi_image_free(img);

    return texture;
}


static void saveDepthTexture(GLuint fbo, std::string filename) {
    int width = SHADOW_WIDTH; // Shadow map width
    int height = SHADOW_HEIGHT; // Shadow map height
    int channels = 3;

    std::vector<float> depth(width * height);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<unsigned char> img(width * height * 3);
    for (int i = 0; i < width * height; ++i) {
        img[3 * i] = img[3 * i + 1] = img[3 * i + 2] = static_cast<unsigned char>(depth[i] * 255.0f);
    }

    stbi_write_png(filename.c_str(), width, height, channels, img.data(), width * channels);
}

bool saveDepthMap = false;


struct Skybox {
    glm::vec3 position; // Position of the box
    glm::vec3 scale; // Size of the box in each axis

    GLfloat vertex_buffer_data[72] = {
        // Vertex definition for a canonical box
        // Front face
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,

        // Back face
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,

        // Left face
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        // Right face
        1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,

        // Top face
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        // Bottom face
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
    };

    GLfloat color_buffer_data[72] = {
        // Front, red
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        // Back, yellow
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,

        // Left, green
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        // Right, cyan
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,

        // Top, blue
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Bottom, magenta
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
    };

    GLuint index_buffer_data[36] = {
        // 12 triangle faces of a box
        0, 3, 2,
        0, 2, 1,

        4, 7, 6,
        4, 6, 5,

        8, 11, 10,
        8, 10, 9,

        12, 15, 14,
        12, 14, 13,

        16, 19, 18,
        16, 18, 17,

        20, 23, 22,
        20, 22, 21,
    };

    // TODO: Define UV buffer data
    // ---------------------------
    // ---------------------------

    GLfloat uv_buffer_data[48] = {
        // Front face (positive Z)
        0.5f, 0.666f, // Top right
        0.25f, 0.666f, // Top left
        0.25f, 0.333f, // Bottom left
        0.5f, 0.333f, // Bottom right

        // Back face (negative Z)
        1.0f, 0.666f, // Top right
        0.75f, 0.666f, // Top left
        0.75f, 0.333f, // Bottom left
        1.0f, 0.333f, // Bottom right


        0.75f, 0.666f, // Top left
        0.5f, 0.666f, // Top right
        0.5f, 0.333f, // Bottom right
        0.75f, 0.333f, // Bottom left

        // Left face (negative X) - Correct
        0.25f, 0.666f, // Top right
        0.0f, 0.666f, // Top left
        0.0f, 0.333f, // Bottom left
        0.25f, 0.333f, // Bottom right

        // Right face (positive X) - Correct

        0.5f, 0.333f,
        0.25f, 0.333f, // Top left
        // Top right
        0.25f, 0.0f,
        0.5f, 0.0f, // Bottom right
        // Bottom left

        // Bottom face (negative Y) - Now using the previous Top UV coordinates
        0.5f, 1.0f,
        0.25f, 1.0f, // Top left
        // Top right
        0.25f, 0.666f,
        0.5f, 0.666f, // Bottom right
        // Bottom left

    };

    GLfloat normal_buffer_data[72] = {
        // Front face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Back face
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,

        // Left face
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        // Right face
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        // Top face
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        // Bottom face
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f
    };


    // OpenGL buffers
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint colorBufferID;
    GLuint uvBufferID;
    GLuint textureID;

    //Ligthing
    GLuint normalBufferID;
    GLuint shadowFBO;
    GLuint depthTexture;
    GLuint lightSpaceMatrixID;
    GLuint lightPositionID;
    GLuint lightIntensityID;

    // Shader variable IDs
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    GLuint programID;

    void initialize(glm::vec3 position, glm::vec3 scale, const char *texturePath) {
        // Define scale of the building geometry
        this->position = position;
        this->scale = scale;

        // Create a vertex array object
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);


        // Add shadow mapping setup
        glGenFramebuffers(1, &shadowFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);


        // Create a vertex buffer object to store the vertex data
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);


        // Create normal buffer
        glGenBuffers(1, &normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);


        // Create a vertex buffer object to store the color data
        // TODO:
        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

        // TODO: Create a vertex buffer object to store the UV data
        // --------------------------------------------------------
        // --------------------------------------------------------
        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

        // Create an index buffer object to store the index data that defines triangle faces
        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

        // Create and compile our GLSL program from the shaders
        programID = LoadShadersFromFile("../street/skybox.vert", "../street/skybox.frag");
        if (programID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }

        // Get a handle for our "MVP" uniform
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        lightPositionID = glGetUniformLocation(programID, "lightPosition");
        lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
        lightSpaceMatrixID = glGetUniformLocation(programID, "lightSpaceMatrix");

        // TODO: Load a texture
        textureID = LoadTextureTileBox(texturePath);


        // TODO: Get a handle to texture sampler
        textureSamplerID = glGetUniformLocation(programID, "textureSampler");
    }

    void render(glm::mat4 cameraMatrix) {
        glUseProgram(programID);

        // Original texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);

        // Vertex attributes
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

        // Model transform
        glm::mat4 modelMatrix = glm::mat4();
        modelMatrix = glm::scale(modelMatrix, scale);

        // Set uniforms
        glm::mat4 mvp = cameraMatrix * modelMatrix;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);


        // Draw
        glDrawElements(
            GL_TRIANGLES,
            36,
            GL_UNSIGNED_INT,
            (void *) 0
        );

        // Cleanup
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
    }


    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteBuffers(1, &uvBufferID);
        glDeleteTextures(1, &textureID);
        glDeleteProgram(programID);
        glDeleteBuffers(1, &normalBufferID);
        glDeleteFramebuffers(1, &shadowFBO);
        glDeleteTextures(1, &depthTexture);
    }
};


struct Building {
    glm::vec3 position; // Position of the box
    glm::vec3 scale; // Size of the box in each axis

    GLfloat vertex_buffer_data[72] = {
        // Vertex definition for a canonical box
        // Front face
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,

        // Back face
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,

        // Left face
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        // Right face
        1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,

        // Top face
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        // Bottom face
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,
    };

    GLfloat normal_buffer_data[72] = {
        // Front face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        // Back face
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        // Other faces...
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        // Other faces...
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
    };

    GLfloat color_buffer_data[72] = {
        // Front, red
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        // Back, yellow
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,

        // Left, green
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        // Right, cyan
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,

        // Top, blue
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Bottom, magenta
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
    };

    GLuint index_buffer_data[36] = {
        // 12 triangle faces of a box
        0, 1, 2,
        0, 2, 3,

        4, 5, 6,
        4, 6, 7,

        8, 9, 10,
        8, 10, 11,

        12, 13, 14,
        12, 14, 15,

        16, 17, 18,
        16, 18, 19,

        20, 21, 22,
        20, 22, 23,
    };


    GLfloat uv_buffer_data[48] = {
        // Front
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        // Back
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        // Left
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        // Right
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        // Top - we do not want texture the top
        0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        // Bottom - we do not want texture the bottom
        0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
    };

    // OpenGL buffers
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint colorBufferID;
    GLuint uvBufferID;
    GLuint textureID;

    // Shader variable IDs
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    GLuint programID;
    GLuint lightPositionID;
    GLuint lightColorID;
    GLuint lightIntensityID;
    GLuint cameraPositionID;
    GLuint normalBufferID;
    // Shader variable IDs for shadow mapping
    GLuint lightSpaceMatrixID;
    GLuint shadowMapID;

    void initialize(glm::vec3 position, glm::vec3 scale, const char *textureFilePath) {
        // Define scale of the building geometry
        this->position = position;
        this->scale = scale;

        // Create a vertex array object
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        // Create a vertex buffer object to store the vertex data
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);


        glGenBuffers(1, &normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);


        for (int i = 0; i < 24; ++i) uv_buffer_data[2 * i + 1] *= 5;


        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,
                     GL_STATIC_DRAW);

        // Create an index buffer object to store the index data that defines triangle faces
        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

        // Create and compile our GLSL program from the shaders
        programID = LoadShadersFromFile("../street/box.vert", "../street/box.frag");
        if (programID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }

        // Get a handle for our "MVP" uniform
        mvpMatrixID = glGetUniformLocation(programID, "MVP");


        textureID = LoadTextureTileBox(textureFilePath);

        textureSamplerID = glGetUniformLocation(programID, "textureSampler");
        lightPositionID = glGetUniformLocation(programID, "lightPosition");
        lightColorID = glGetUniformLocation(programID, "lightColor");
        lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
        cameraPositionID = glGetUniformLocation(programID, "cameraPosition");
        // After loading the shader program
        lightSpaceMatrixID = glGetUniformLocation(programID, "lightSpaceMatrix");
        shadowMapID = glGetUniformLocation(programID, "shadowMap");
    }

    void render(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix, GLuint depthMap) {
        glUseProgram(programID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

        // Set the light space matrix uniform
        glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        // Bind the depth map to texture unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(shadowMapID, 1);

        // ----------------------
        // Create the model transformation matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position); // Translate to the building's position
        modelMatrix = glm::scale(modelMatrix, scale); // Scale to the building's dimensions

        // -----------------------

        // Set model-view-projection matrix
        glm::mat4 mvp = cameraMatrix * modelMatrix;


        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        glUniformMatrix4fv(glGetUniformLocation(programID, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);
        glUniformMatrix3fv(glGetUniformLocation(programID, "normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);


        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);

        glEnableVertexAttribArray(3);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);


        glUniform3fv(lightPositionID, 1, &lightPosition[0]);
        glUniform3fv(lightColorID, 1, &lightColor[0]);
        glUniform1f(lightIntensityID, lightIntensity);
        glUniform3fv(cameraPositionID, 1, &cameraPosition[0]);

        // Pass light direction to the shader
        glUniform3fv(glGetUniformLocation(programID, "lightDirection"), 1, &lightDirection[0]);


        // Draw the box
        glDrawElements(
            GL_TRIANGLES, // mode
            36, // number of indices
            GL_UNSIGNED_INT, // type
            (void *) 0 // element array buffer offset
        );

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    void renderDepth(GLuint shaderProgramID, glm::mat4 lightSpaceMatrix) {
        glUseProgram(shaderProgramID);

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "lightSpaceMatrix"), 1, GL_FALSE,
                           &lightSpaceMatrix[0][0]);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "modelMatrix"), 1, GL_FALSE, &modelMatrix[0][0]);

        glBindVertexArray(vertexArrayID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void *) 0);

        glDisableVertexAttribArray(0);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteProgram(programID);
    }
};


static std::vector<Sand> sandChunks;

void updateSandChunks(const glm::vec3 &cameraPosition) {
    const float chunkSize = 1000.0f;
    const float baseHeight = -132.0f;
    const int renderDistance = 2; // Number of chunks to render in each direction

    // Calculate current chunk coordinates
    int currentChunkX = static_cast<int>(floor(cameraPosition.x / chunkSize));
    int currentChunkZ = static_cast<int>(floor(cameraPosition.z / chunkSize));

    // Check which chunks should exist
    for (int x = currentChunkX - renderDistance; x <= currentChunkX + renderDistance; x++) {
        for (int z = currentChunkZ - renderDistance; z <= currentChunkZ + renderDistance; z++) {
            glm::vec3 chunkPosition(x * chunkSize, baseHeight, z * chunkSize);

            // Check if chunk already exists
            bool chunkExists = false;
            for (const auto &chunk: sandChunks) {
                if (chunk.position == chunkPosition) {
                    chunkExists = true;
                    break;
                }
            }

            // Create new chunk if it doesn't exist
            if (!chunkExists) {
                Sand newChunk;
                newChunk.initialize(chunkPosition,
                                    glm::vec3(chunkSize, 1.0f, chunkSize),
                                    "../street/sand.jpg");
                sandChunks.push_back(newChunk);
            }
        }
    }
}





int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context

    window = glfwCreateWindow(1024, 768, "JaedonPaget", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to open a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //Hide and capture cursor
    glfwSetCursorPosCallback(window, mouse_callback);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetKeyCallback(window, key_callback);

    // Load OpenGL functions
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Create the depth framebuffer
    glGenFramebuffers(1, &depthMapFBO);

    // Create the depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint depthShaderProgramID = LoadShadersFromFile("../street/depth.vert", "../street/depth.frag");

    GLuint particleShaderProgram = LoadShadersFromFile("../street/particle.vert", "../street/particle.frag");

    std::vector<Building> buildings3;
    std::vector<Building> walls;
    std::vector<Building> edgeBuildings;
    std::vector<Building> cornerBuildings;


    Skybox skybox;
    skybox.initialize(glm::vec3(0, 0, 0), glm::vec3(1200, 1200, 1200), "../street/sky.png");


    float floorSize = 1000.0f;
    Floor floor;
    floor.initialize(glm::vec3(0.0f, -130.0f, 0.0f), glm::vec3(floorSize, 1.0f, floorSize),
                     "../street/road_texture.jpg");



    updateSandChunks(cameraPosition);


    Bot bot;
    bot.initialize();
    static double lastTime = glfwGetTime();
    float time = 0.0f;


    Building wallNorth, wallSouth, wallEast, wallWest;
    float wallHeight = 50.0f;
    float wallThickness = 10.0f;

    wallNorth.initialize(glm::vec3(0.0f, -100.0f, -1000),
                         glm::vec3(floorSize, wallHeight, wallThickness), "../street/warning.png");
    // South wall
    wallSouth.initialize(
        glm::vec3(0.0f, -100.0f, 1000),
        glm::vec3(floorSize, wallHeight, wallThickness), "../street/warning.png"
    );

    // East wall
    wallEast.initialize(
        glm::vec3(1000, -100.0f, 0.0f),
        glm::vec3(wallThickness, wallHeight, floorSize), "../street/warning.png"
    );

    // West wall
    wallWest.initialize(
        glm::vec3(-1000, -100.0f, 0.0f),
        glm::vec3(wallThickness, wallHeight, floorSize), "../street/warning.png"
    );
    // Add walls to vector
    walls.push_back(wallNorth);
    walls.push_back(wallSouth);
    walls.push_back(wallEast);
    walls.push_back(wallWest);

    Building sign;

    sign.initialize(glm::vec3(800, 0, 0), glm::vec3(2, 130, 100), "../street/warning.png");



    float cornerSize = 80.0f;
    float cornerHeight = 700.0f;
    float wallGap = 30.0f; // Gap between buildings and walls

    // Four corner buildings
    Building corner1, corner2, corner3, corner4;
    corner1.initialize(
        glm::vec3(-900.0f + wallGap, -130.0f, -900.0f + wallGap),
        glm::vec3(cornerSize, cornerHeight, cornerSize),
        "../street/nightCity-facade.jpg"
    );

    corner2.initialize(
        glm::vec3(900.0f - wallGap, -130.0f, -900.0f + wallGap),
        glm::vec3(cornerSize, cornerHeight, cornerSize),
        "../street/nightCity-facade.jpg"
    );

    corner3.initialize(
        glm::vec3(-900.0f + wallGap, -130.0f, 900.0f - wallGap),
        glm::vec3(cornerSize, cornerHeight, cornerSize),
        "../street/nightCity-facade.jpg"
    );

    corner4.initialize(
        glm::vec3(900.0f - wallGap, -130.0f, 900.0f - wallGap),
        glm::vec3(cornerSize, cornerHeight, cornerSize),
        "../street/nightCity-facade.jpg"
    );

    cornerBuildings.push_back(corner1);
    cornerBuildings.push_back(corner2);
    cornerBuildings.push_back(corner3);
    cornerBuildings.push_back(corner4);

    // Get positions from corner buildings for particle systems
    glm::vec3 corner1Pos = glm::vec3(-900.0f + wallGap, cornerHeight - 130.0f, -900.0f + wallGap);
    glm::vec3 corner2Pos = glm::vec3(900.0f - wallGap, cornerHeight - 130.0f, -900.0f + wallGap);
    glm::vec3 corner3Pos = glm::vec3(-900.0f + wallGap, cornerHeight - 130.0f, 900.0f - wallGap);
    glm::vec3 corner4Pos = glm::vec3(900.0f - wallGap, cornerHeight - 130.0f, 900.0f - wallGap);



    // Create particle systems for each corner
    ParticleSystem particleSystem1(200, particleShaderProgram);
    ParticleSystem particleSystem2(200, particleShaderProgram);
    ParticleSystem particleSystem3(200, particleShaderProgram);
    ParticleSystem particleSystem4(200, particleShaderProgram);

    // Initialize each particle system at corner building positions
    particleSystem1.initialize(corner1Pos, corner1Pos + glm::vec3(0.0f, 50.0f, 0.0f));
    particleSystem2.initialize(corner2Pos, corner2Pos + glm::vec3(0.0f, 50.0f, 0.0f));
    particleSystem3.initialize(corner3Pos, corner3Pos + glm::vec3(0.0f, 50.0f, 0.0f));
    particleSystem4.initialize(corner4Pos, corner4Pos + glm::vec3(0.0f, 50.0f, 0.0f));




    // Create edge buildings
    float edgeSpacing = 200.0f;
    // North edge
    for (int i = 0; i < 8; i++) {
        Building b;
        float randomHeight = 150.0f + static_cast<float>(rand()) / RAND_MAX * 200.0f;
        float randomWidth = 40.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f;
        float xPos = -800.0f + (i * edgeSpacing);
        float zPos = -900.0f + wallGap;
        b.initialize(
            glm::vec3(xPos, -130.0f, zPos),
            glm::vec3(randomWidth, randomHeight, randomWidth),
            "../street/nightCity-facade.jpg"
        );
        edgeBuildings.push_back(b);
    }

    // West edge
    for (int i = 0; i < 8; i++) {
        Building b;
        float randomHeight = 150.0f + static_cast<float>(rand()) / RAND_MAX * 200.0f;
        float randomWidth = 40.0f + static_cast<float>(rand()) / RAND_MAX * 30.0f;
        float xPos = -900.0f + wallGap;
        float zPos = -800.0f + (i * edgeSpacing);
        b.initialize(
            glm::vec3(xPos, -130.0f, zPos),
            glm::vec3(randomWidth, randomHeight, randomWidth),
            "../street/nightCity-facade.jpg"
        );
        edgeBuildings.push_back(b);
    }




    // Create buildings
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Building b;
            float spacing = 150.0f;
            float randomOffsetX = static_cast<float>(rand()) / RAND_MAX * 50.0f;
            float randomOffsetZ = static_cast<float>(rand()) / RAND_MAX * 50.0f;
            float xPos = -400.0f + (col * spacing) + randomOffsetX;
            float zPos = -400.0f + (row * spacing) + randomOffsetZ;
            float height = 100.0f + static_cast<float>(rand()) / RAND_MAX * 200.0f;
            float buildingWidth = 30.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
            float buildingDepth = 30.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
            b.initialize(glm::vec3(xPos, -40, zPos), glm::vec3(buildingWidth, height, buildingDepth),
                         "../street/nightCity-facade.jpg");
            buildings3.push_back(b);
        }
    }


    // Camera setup
    glm::mat4 viewMatrix, projectionMatrix;
    glm::float32 FoV = 90;
    glm::float32 zNear = 0.1f;
    glm::float32 zFar = 2000.0f;
    projectionMatrix = glm::perspective(glm::radians(FoV), (float) windowWidth / windowHeight, zNear, zFar);

    float fTime = 0.0f;			// Time for measuring fps
    unsigned long frames = 0;

    do {
        // Shadow mapping pass
        float near_plane = 10.0f, far_plane = 1800.0f;
        float orthoSize = 1000.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPosition, lightLookAt, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // Update light information
        lightDirection = glm::normalize(lightLookAt - lightPosition);
        sunLightInfo.position = lightPosition;
        sunLightInfo.color = lightColor;
        sunLightInfo.intensity = lightIntensity;
        sunLightInfo.direction = lightDirection;

        // First render pass - shadow mapping
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthShaderProgramID);

        // Set the uniform for the light space matrix
        glUniformMatrix4fv(glGetUniformLocation(depthShaderProgramID, "lightSpaceMatrix"), 1, GL_FALSE,
                           &lightSpaceMatrix[0][0]);
        for (Building &building: buildings3) {
            building.renderDepth(depthShaderProgramID, lightSpaceMatrix);
        }

        for (Building &wall: walls) {
            wall.renderDepth(depthShaderProgramID, lightSpaceMatrix);
        }

        for (Building &building: edgeBuildings) {
            building.renderDepth(depthShaderProgramID, lightSpaceMatrix);
        }
        for (Building &building: cornerBuildings) {
            building.renderDepth(depthShaderProgramID, lightSpaceMatrix);
        }

        sign.renderDepth(depthShaderProgramID, lightSpaceMatrix);


        // Main rendering pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update view and projection matrices
        viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
        glm::mat4 vp = projectionMatrix * viewMatrix;

        glm::mat4 skybox2Matrix = glm::mat4(1.0f);
        skybox2Matrix = glm::translate(skybox2Matrix, cameraPosition);
        skybox2Matrix = glm::scale(skybox2Matrix, glm::vec3(2000, 2000, 2000));

        // Modify the view matrix to remove translation
        glm::mat4 viewWithoutTranslation = glm::mat4(glm::mat3(viewMatrix));
        glm::mat4 skyboxVP = projectionMatrix * viewWithoutTranslation;

        // Render skybox
        glDisable(GL_DEPTH_TEST);
        skybox.render(skyboxVP);
        glEnable(GL_DEPTH_TEST);


        // Render floor and buildings
        floor.render(vp, lightSpaceMatrix, depthMap, sunLightInfo, cameraPosition);
        updateSandChunks(cameraPosition);
        for (auto &chunk: sandChunks) {
            chunk.render(vp, lightSpaceMatrix, depthMap, sunLightInfo, cameraPosition);
        }


        for (Building &building: buildings3) {
            building.render(vp, lightSpaceMatrix, depthMap);
        }

        for (Building &wall: walls) {
            wall.render(vp, lightSpaceMatrix, depthMap);
        }

        for (Building &building: edgeBuildings) {
            building.render(vp, lightSpaceMatrix, depthMap);
        }
        for (Building &building: cornerBuildings) {
            building.render(vp, lightSpaceMatrix, depthMap);
        }

        sign.render(vp, lightSpaceMatrix, depthMap);


        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        if (playAnimation) {
            time += deltaTime * playbackSpeed;
            bot.update(time);
        }

        glm::mat4 botTransform = glm::mat4(1.0f);
        botTransform = glm::translate(botTransform, glm::vec3(0.0f, -330.0f, 0.0f));
        botTransform = glm::scale(botTransform, glm::vec3(8.0f, 6.0f, 8.0f));
        bot.render(vp * botTransform);


        // Update particles
        particleSystem1.update(deltaTime, corner1Pos, corner1Pos + glm::vec3(0.0f, 50.0f, 0.0f));
        particleSystem2.update(deltaTime, corner2Pos, corner2Pos + glm::vec3(0.0f, 50.0f, 0.0f));
        particleSystem3.update(deltaTime, corner3Pos, corner3Pos + glm::vec3(0.0f, 50.0f, 0.0f));
        particleSystem4.update(deltaTime, corner4Pos, corner4Pos + glm::vec3(0.0f, 50.0f, 0.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Render particles
        particleSystem1.render(projectionMatrix * viewMatrix);
        particleSystem2.render(projectionMatrix * viewMatrix);
        particleSystem3.render(projectionMatrix * viewMatrix);
        particleSystem4.render(projectionMatrix * viewMatrix);

        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);



        // FPS tracking
        // Count number of frames over a few seconds and take average
        frames++;
        fTime += deltaTime;
        if (fTime > 2.0f) {
            float fps = frames / fTime;
            frames = 0;
            fTime = 0;

            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << "JaedonPaget | Frames per second (FPS): " << fps;
            glfwSetWindowTitle(window, stream.str().c_str());
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (!glfwWindowShouldClose(window));

    // Cleanup
    for (Building &building: buildings3) {
        building.cleanup();
    }
    for (Building &wall: walls) {
        wall.cleanup();
    }
    for (Building &building: edgeBuildings) {
        building.cleanup();
    }
    for (Building &building: cornerBuildings) {
        building.cleanup();
    }
    skybox.cleanup();
    floor.cleanup();
    bot.cleanup();
    for (auto &chunk: sandChunks) {
        chunk.cleanup();
    }
    sign.cleanup();
    particleSystem1.cleanup();
    particleSystem2.cleanup();
    particleSystem3.cleanup();
    particleSystem4.cleanup();



    glfwTerminate();
    return 0;
}


// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    float cameraSpeed = 10.0f; // Adjust for faster/slower movement

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W)
            cameraPosition += cameraSpeed * cameraFront;
        if (key == GLFW_KEY_S)
            cameraPosition -= cameraSpeed * cameraFront;
        if (key == GLFW_KEY_A)
            cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (key == GLFW_KEY_D)
            cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}


static void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront.y = sin(glm::radians(pitch));
    cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(cameraFront);
}
