#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 texCoord;

// convertito da vec2 a vec3 perchè ora tratto skybox come oggetto tridimensionale
layout(location = 2) out vec3 fragTexCoord;

void main() {
	fragTexCoord = pos;
	vec4 posv = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);
	gl_Position = posv.xyww;


	//gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos, 1.0);

	//fragTexCoord = texCoord;
}