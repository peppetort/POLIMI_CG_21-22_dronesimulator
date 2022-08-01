#version 450

layout(binding = 1) uniform samplerCube cubeMapTexture;

layout(location = 2) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(cubeMapTexture, fragTexCoord);

}