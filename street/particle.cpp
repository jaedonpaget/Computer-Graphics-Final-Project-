#include "particle.h"
#include <cstdlib>
#include <iostream>

ParticleSystem::ParticleSystem(int particleMax, GLuint shaderProgramID) : shaderProgramID(shaderProgramID) {
    particles.resize(particleMax);

    // Generate VAO and VBO
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Particle) * particles.size(), nullptr, GL_DYNAMIC_DRAW);

    // Enable vertex attributes
    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));

    glEnableVertexAttribArray(1); // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));

    glEnableVertexAttribArray(2); // Size
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, size));

    glBindVertexArray(0);

}

// Destructor
ParticleSystem::~ParticleSystem() {
    cleanup();
}

// Initialize particles
void ParticleSystem::initialize(glm::vec3 start, glm::vec3 end) {
    for (size_t i = 0; i < particles.size(); ++i) {
        float t = static_cast<float>(i) / particles.size();
        particles[i].position = glm::mix(start, end, t);
        particles[i].velocity = glm::normalize(end - start) * 10.0f;
        particles[i].color = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);
        particles[i].size = 10.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f;
        particles[i].lifeTime = t;

        // Assign random offset
        particles[i].randomOffset = glm::vec3((static_cast<float>(rand()) / RAND_MAX - 0.5f) * 200.0f, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 50.0f, (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 200.0f );
    }
}


void ParticleSystem::update(float deltaTime, glm::vec3 start, glm::vec3 end) {
    for (auto &particle : particles) {
        particle.lifeTime += deltaTime * 0.1f;
        if (particle.lifeTime >= 1.0f) {
            particle.lifeTime -= 1.0f;
        }

        glm::vec3 basePosition = glm::mix(start, end, particle.lifeTime);

        glm::vec3 offset = particle.randomOffset;
        particle.position = basePosition + offset;
    }

    // Update particle buffer
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Particle) * particles.size(), particles.data());
}



// Render particles
void ParticleSystem::render(glm::mat4 vpMatrix) {
    glUseProgram(shaderProgramID);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "vpMatrix"), 1, GL_FALSE, &vpMatrix[0][0]);

    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);
}


// Cleanup resources
void ParticleSystem::cleanup() {
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &particleVAO);
}
