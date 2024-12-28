// sign.frag
#version 330 core

in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 Normal_worldspace;

out vec4 color;

uniform sampler2D textureSampler;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

void main() {
    // Basic texture sampling
    vec4 textureColor = texture(textureSampler, UV);

    // Basic lighting calculation
    vec3 normal = normalize(Normal_worldspace);
    vec3 lightDir = normalize(lightPosition - Position_worldspace);

    // Ambient light
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);

    // Diffuse light
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Final color
    vec3 lighting = ambient + diffuse;
    color = vec4(lighting * textureColor.rgb, textureColor.a);
}
