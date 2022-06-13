// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

// The uniform buffer object used in this example
struct globalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};

struct SkyBoxUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 model;
};
// Camera
glm::vec3 CamAng = glm::vec3(0.0f);
glm::vec3 CamPos = glm::vec3(0.0f, -1.6f, 5.0f);


// MAIN ! 
class MyProject : public BaseProject {
protected:
	// Here you list all the Vulkan objects you need:

	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLglobal;
	DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	Model M_Terrain;
	Texture T_Terrain;
	DescriptorSet DS_Terrain;	// instance DSLobj

	Model M_Palm;
	Texture T_Palm;
	Texture test;
	DescriptorSet DS_Palm1;	// instance DSLobj
	//DescriptorSet DS_Palm2;	// instance DSLobj
	//DescriptorSet DS_Palm3;	// instance DSLobj


	DescriptorSet DS_global;




	DescriptorSetLayout SkyBoxDescriptorSetLayout; // for skybox
	Pipeline SkyBoxPipeline;		// for skybox
	Model M_SkyBox;
	Texture T_SkyBox;
	DescriptorSet DS_SkyBox;	// instance SkyBoxDescriptorSetLayout

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "My Project";
		initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Descriptor pool sizes
		uniformBlocksInPool = 6;
		texturesInPool = 5;
		setsInPool = 6;
	}

	// Here you load and setup all your Vulkan objects
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLobj.init(this, {
			// this array contains the binding:
			// first  element : the binding number
			// second element : the time of element (buffer or texture)
			// third  element : the pipeline stage where it will be used
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		DSLglobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
			});
		DS_global.init(this, &DSLglobal, {
			{0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/vert.spv", "shaders/frag.spv", { &DSLglobal, &DSLobj });

		// Models, textures and Descriptors (values assigned to the uniforms)
		M_Terrain.init(this, "models/Terrain.obj");
		T_Terrain.init(this, "textures/PaloDuroPark.jpg");
		DS_Terrain.init(this, &DSLobj, {
			// the second parameter, is a pointer to the Uniform Set Layout of this set
			// the last parameter is an array, with one element per binding of the set.
			// first  elmenet : the binding number
			// second element : UNIFORM or TEXTURE (an enum) depending on the type
			// third  element : only for UNIFORMs, the size of the corresponding C++ object
			// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &T_Terrain}
			});

		M_Palm.init(this, "models/MapleTree.obj");
		T_Palm.init(this, "textures/maple_bark.png");
		DS_Palm1.init(this, &DSLobj, {
					{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_Palm}
			});
		/*DS_Palm2.init(this, &DSLobj, {
					{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_Palm}
			});
		DS_Palm3.init(this, &DSLobj, {
					{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_Palm}
			});*/


		SkyBoxDescriptorSetLayout.init(this, {
			// this array contains the binding:
			// first  element : the binding number
			// second element : the time of element (buffer or texture)
			// third  element : the pipeline stage where it will be used
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		SkyBoxPipeline.init(this, "shaders/SkyBoxvert.spv", "shaders/SkyBoxfrag.spv", { &SkyBoxDescriptorSetLayout});


		M_SkyBox.init(this, "models/SkyBoxCube.obj");
		T_SkyBox.init(this, "textures/skybox.png");
		DS_SkyBox.init(this, &SkyBoxDescriptorSetLayout, {
					{0, UNIFORM, sizeof(SkyBoxUniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_SkyBox}
			});
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
		DS_Terrain.cleanup();
		T_Terrain.cleanup();
		M_Terrain.cleanup();

		DS_Palm1.cleanup();
		/*DS_Palm2.cleanup();
		DS_Palm3.cleanup();*/
		M_Palm.cleanup();
		T_Palm.cleanup();

		DS_global.cleanup();

		P1.cleanup();
		DSLglobal.cleanup();
		DSLobj.cleanup();



		DS_SkyBox.cleanup();
		T_SkyBox.cleanup();
		M_SkyBox.cleanup();

		SkyBoxPipeline.cleanup();
		SkyBoxDescriptorSetLayout.cleanup();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.graphicsPipeline);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);


		VkBuffer vertexBuffers[] = { M_Terrain.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, M_Terrain.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_Terrain.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_Terrain.indices.size()), 1, 0, 0, 0);


		VkBuffer vertexBuffers3[] = { M_Palm.vertexBuffer };
		VkDeviceSize offsets3[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers3, offsets3);
		vkCmdBindIndexBuffer(commandBuffer, M_Palm.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_Palm1.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_Palm.indices.size()), 1, 0, 0, 0);

		/*vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_Palm2.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_Palm.indices.size()), 1, 0, 0, 0);

		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_Palm3.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_Palm.indices.size()), 1, 0, 0, 0);*/

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			SkyBoxPipeline.graphicsPipeline);

		VkBuffer vertexBuffers2[] = { M_SkyBox.vertexBuffer };
		VkDeviceSize offsets2[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
		vkCmdBindIndexBuffer(commandBuffer, M_SkyBox.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			SkyBoxPipeline.pipelineLayout, 0, 1, &DS_SkyBox.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_SkyBox.indices.size()), 1, 0, 0, 0);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		static float lastTime = 0.0f;
		float deltaT = time - lastTime;		

		const float ROT_SPEED = glm::radians(60.0f);
		float MOVE_SPEED = 20.0f;
		
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			CamAng.y += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			CamAng.y -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			CamAng.x += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			CamAng.x -= deltaT * ROT_SPEED;
		}

		lastTime = time; //Frame rate independance


		glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
			glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {//Sprint
			MOVE_SPEED = 2 * MOVE_SPEED;
		}

		if (glfwGetKey(window, GLFW_KEY_A)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_D)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_S)) {
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_W)) {
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_F)) {
			CamPos -= MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_R)) {
			CamPos += MOVE_SPEED * glm::vec3(0, 1, 0) * deltaT;
		}

		glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -CamPos);



		globalUniformBufferObject gubo{};
		UniformBufferObject ubo{};

		void* data;

		gubo.view = CamMat;
		gubo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 50.0f); //FOV
		gubo.proj[1][1] *= -1;

		vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);


		// For the Terrain
		ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f)) *
			glm::translate(glm::mat4(1.0f), glm::vec3(-20.0f, -4.0f, 10.0f)) * 
			glm::rotate(glm::mat4(1.0f),
			glm::radians(-90.0f),
			glm::vec3(1.0f, 0.0f, 0.0f));;
		vkMapMemory(device, DS_Terrain.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_Terrain.uniformBuffersMemory[0][currentImage]);

		// For the tree1
		ubo.model =
			glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, -13.5f, -0.15f))*
			glm::scale(glm::mat4(1.0f),glm::vec3(0.1f, 0.1f, 0.1f));
		vkMapMemory(device, DS_Palm1.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_Palm1.uniformBuffersMemory[0][currentImage]);

		//// For the tree2
		//ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.93f, -0.15f));
		//vkMapMemory(device, DS_Palm2.uniformBuffersMemory[0][currentImage], 0,
		//	sizeof(ubo), 0, &data);
		//memcpy(data, &ubo, sizeof(ubo));
		//vkUnmapMemory(device, DS_Palm2.uniformBuffersMemory[0][currentImage]);

		//// For the tree3
		//ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.15f, 0.93f, -0.15f));
		//vkMapMemory(device, DS_Palm3.uniformBuffersMemory[0][currentImage], 0,
		//	sizeof(ubo), 0, &data);
		//memcpy(data, &ubo, sizeof(ubo));
		//vkUnmapMemory(device, DS_Palm3.uniformBuffersMemory[0][currentImage]);

		SkyBoxUniformBufferObject subo{};

		void* dataSB;

		subo.model = glm::mat4(1.0f);
		subo.view = glm::transpose(glm::mat4(CamDir));
		subo.proj = glm::perspective(glm::radians(45.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 50.0f) * subo.view;
		subo.proj[1][1] *= -1;

		// For the SkyBox
		vkMapMemory(device, DS_SkyBox.uniformBuffersMemory[0][currentImage], 0,
			sizeof(subo), 0, &dataSB);
		memcpy(dataSB, &subo, sizeof(subo));
		vkUnmapMemory(device, DS_SkyBox.uniformBuffersMemory[0][currentImage]);

	}
};

// This is the main: probably you do not need to touch this!
int main() {
	MyProject app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}