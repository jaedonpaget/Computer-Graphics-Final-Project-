#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

layout(location = 3) in uvec4 jointIndices;
layout(location = 4) in vec4 jointWeights;

// Output data, to be interpolated for each fragment
out vec3 worldPosition;
out vec3 worldNormal;

uniform mat4 MVP;
uniform mat4 jointMatrices[25];

void main() {
    //skinning tranformation
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    //Need to loop over the four possible influences on the joints
    for(int i=0; i<4; i++){
        float weight = jointWeights[i];
        if(weight > 0.0){
            //Get the joint index from the vertex
            uint jointIndex = jointIndices[i];

            mat4 jointMatrix = jointMatrices[jointIndex];

            //Apply skiing to the position
            skinnedPosition += weight * (jointMatrix * vec4(vertexPosition, 1.0));

            //Skin the normal
            mat3 jointMatrix3 = mat3(jointMatrix);
            skinnedNormal += weight * (jointMatrix3 * vertexNormal);
        }
    }

    worldPosition = vec3(skinnedPosition);
    worldNormal = normalize(skinnedNormal);

    // Transform vertex
    gl_Position =  MVP * skinnedPosition;
}
