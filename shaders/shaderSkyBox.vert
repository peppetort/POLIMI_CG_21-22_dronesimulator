#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
 	mat4 view;
	mat4 proj;
	mat4 model;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;

void main()
{
    fragTexCoord = inPosition;
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
}  