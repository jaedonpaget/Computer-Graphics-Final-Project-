
#ifndef BOT_H
#define BOT_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <tinygltf-2.9.3/tiny_gltf.h>


#include <vector>
#include <map>
#include <string>


class Bot {
public:
    Bot();
    ~Bot();

    void initialize();
    void update(float time);
    void render(glm::mat4 cameraMatrix);
    void cleanup();

private:
    // Shader variable IDs
    GLuint mvpMatrixID;
    GLuint jointMatricesID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    GLuint programID;

    tinygltf::Model model;

    // Each VAO corresponds to each mesh primitive in the GLTF model
    struct PrimitiveObject {
        GLuint vao;
        std::map<int, GLuint> vbos;
    };
    std::vector<PrimitiveObject> primitiveObjects;

    // Skinning
    struct SkinObject {
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<glm::mat4> globalJointTransforms;
        std::vector<glm::mat4> jointMatrices;
    };
    std::vector<SkinObject> skinObjects;

    // Animation
    struct SamplerObject {
        std::vector<float> input;
        std::vector<glm::vec4> output;
        int interpolation;
    };

    struct ChannelObject {
        int sampler;
        std::string targetPath;
        int targetNode;
    };

    struct AnimationObject {
        std::vector<SamplerObject> samplers;
    };
    std::vector<AnimationObject> animationObjects;

    glm::mat4 inverseRootTransform;

    // Private member functions
    glm::mat4 getNodeTransform(const tinygltf::Node& node);
    void computeLocalNodeTransform(const tinygltf::Model& model, int nodeIndex, std::vector<glm::mat4>& localTransforms);
    void computeGlobalNodeTransform(const tinygltf::Model& model, const std::vector<glm::mat4>& localTransforms, int nodeIndex, const glm::mat4& parentTransform, std::vector<glm::mat4>& globalTransforms);
    std::vector<SkinObject> prepareSkinning(const tinygltf::Model& model);
    int findKeyframeIndex(const std::vector<float>& times, float animationTime);
    std::vector<AnimationObject> prepareAnimation(const tinygltf::Model& model);
    void updateAnimation(const tinygltf::Model& model, const tinygltf::Animation& anim, const AnimationObject& animationObject, float time, std::vector<glm::mat4>& nodeTransforms);
    void updateSkinning(const tinygltf::Skin& skin, const std::vector<glm::mat4>& nodeTransforms);
    bool loadModel(tinygltf::Model& model, const char* filename);
    void bindMesh(std::vector<PrimitiveObject>& primitiveObjects, tinygltf::Model& model, tinygltf::Mesh& mesh);
    void bindModelNodes(std::vector<PrimitiveObject>& primitiveObjects, tinygltf::Model& model, tinygltf::Node& node);
    std::vector<PrimitiveObject> bindModel(tinygltf::Model& model);
    void drawMesh(const std::vector<PrimitiveObject>& primitiveObjects, tinygltf::Model& model, tinygltf::Mesh& mesh);
    void drawModelNodes(const std::vector<PrimitiveObject>& primitiveObjects, tinygltf::Model& model, tinygltf::Node& node);
    void drawModel(const std::vector<PrimitiveObject>& primitiveObjects, tinygltf::Model& model);

};

#endif //BOT_H
