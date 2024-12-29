#ifndef PARTICLE_H
#define PARTICLE_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

struct Particle {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 velocity;
    float size;
    float lifeTime;
    glm::vec3 randomOffset;

};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    GLuint particleVAO, particleVBO;
    GLuint shaderProgramID;

public:
    ParticleSystem(int particleMax, GLuint shaderProgramID);
    ~ParticleSystem();
    void initialize(glm::vec3 start, glm::vec3 end);
    void update(float deltaTime, glm::vec3 start, glm::vec3 end);
    void render(glm::mat4 vpMatrix);
    void cleanup();

};

#endif //PARTICLE_H
