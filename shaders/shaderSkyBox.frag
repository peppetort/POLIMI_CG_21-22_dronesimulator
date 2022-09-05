#version 450

// convertito da sampler2d a samplerCube
layout(binding = 1) uniform samplerCube cubeMapTexture;

// convertito da vec2 a vec3 perchè ora tratto skybox come ogetto tridimensionale
layout(location = 2) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // texture mapping: automaticamente posiziona le texture sull'oggetto
    // segundo le indicazioni di fragTextCoord
    outColor = texture(cubeMapTexture, fragTexCoord);

}