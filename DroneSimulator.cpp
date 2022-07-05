#include "Models.hpp"

// MAIN !
class DroneSimulator : public BaseProject {

private:
    glm::vec3 cameraPosition = glm::vec3(0.f);
    std::map<int, int> keys_status = {
            {GLFW_KEY_A,     GLFW_RELEASE},
            {GLFW_KEY_S,     GLFW_RELEASE},
            {GLFW_KEY_D,     GLFW_RELEASE},
            {GLFW_KEY_W,     GLFW_RELEASE},
            {GLFW_KEY_UP,    GLFW_RELEASE},
            {GLFW_KEY_DOWN,  GLFW_RELEASE},
            {GLFW_KEY_RIGHT, GLFW_RELEASE},
            {GLFW_KEY_LEFT,  GLFW_RELEASE},
    };

protected:
    // Here you list all the Vulkan objects you need:

    // Descriptor Layouts [what will be passed to the shaders]
    DescriptorSetLayout DSLglobal;
    DescriptorSetLayout DSLobj;

    DescriptorSet DS_global;

    //Terrain
    Pipeline terrainPipeline;
    Terrain terrain = Terrain(this, &DSLobj, &terrainPipeline);

    //Drone
    Pipeline dronePipeline;
    Drone drone = Drone(this, &DSLobj, &dronePipeline, &cameraPosition, &terrain);

    //Skybox
    DescriptorSetLayout SkyBoxDescriptorSetLayout; // for skybox
    Pipeline skyBoxPipeline;        // for skybox
    BaseModel skyboxBaseModel = BaseModel(this, &SkyBoxDescriptorSetLayout, &skyBoxPipeline);

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, title and initial background
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "Drone simulator";
        initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};

        // Descriptor pool sizes
        uniformBlocksInPool = 8;
        texturesInPool = 7;
        setsInPool = 8;
    }

    // Here you load and setup all your Vulkan objects
    void localInit(bool first = true) {
        // Descriptor Layouts [what will be passed to the shaders]
        DSLobj.init(this, {
                // this array contains the binding:
                // first  element : the binding number
                // second element : the time of element (buffer or texture)
                // third  element : the pipeline stage where it will be used
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

        DSLglobal.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
        });
        DS_global.init(this, &DSLglobal, {
                {0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
        });

        // Pipelines [Shader couples]
        // The last array, is a vector of pointer to the layouts of the sets that will
        // be used in this pipeline. The first element will be set 0, and so on...

        // Terrain
        terrainPipeline.init(this, "shaders/shaderTerrainVert.spv", "shaders/shaderTerrainFrag.spv",
                             {&DSLglobal, &DSLobj},first, false);
        
        terrain.terrainBaseModel.init("models/Terrain.obj", "textures/terrain.png",first);

        // Drone
        dronePipeline.init(this, "shaders/shaderDroneVert.spv", "shaders/shaderDroneFrag.spv", {&DSLglobal, &DSLobj}, first,
                           false);

        drone.droneBaseModel.init("models/Drone.obj", "textures/drone.png",first);
        for (auto& i : drone.fanBaseModelList) {
            i.init("models/Fan.obj","textures/fan.png", first);//Texture to avoid errors
        }
        

        // Skybox
        SkyBoxDescriptorSetLayout.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });
        skyBoxPipeline.init(this, "shaders/shaderSkyBoxVert.spv", "shaders/shaderSkyBoxFrag.spv",
                            {&SkyBoxDescriptorSetLayout}, first, true);
        skyboxBaseModel.init("models/SkyBox.obj", "textures/fog.png", first, true);

    }

    // Here you destroy all the objects you created!
    void localCleanup(bool definitive = true) {

        terrain.terrainBaseModel.cleanUp(definitive);
        drone.droneBaseModel.cleanUp(definitive);
        for (auto &i: drone.fanBaseModelList) {
            i.cleanUp(definitive);
        }

        DS_global.cleanup();

        terrainPipeline.cleanup();
        dronePipeline.cleanup();
        DSLglobal.cleanup();
        DSLobj.cleanup();

        skyboxBaseModel.cleanUp(definitive);
        skyBoxPipeline.cleanup();
        SkyBoxDescriptorSetLayout.cleanup();
    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

        // Bind terrainPipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          terrainPipeline.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                terrainPipeline.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                0, nullptr);

        // Terrain
        terrain.terrainBaseModel.populateCommandBuffer(&commandBuffer, currentImage, 1);

        // Bind dronePipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          dronePipeline.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                dronePipeline.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                0, nullptr);

        // Drone
        drone.droneBaseModel.populateCommandBuffer(&commandBuffer, currentImage, 1);
        for (auto &fanBaseModel: drone.fanBaseModelList) {
            fanBaseModel.populateCommandBuffer(&commandBuffer, currentImage, 1);
        }

        //Pipeline for skybox
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          skyBoxPipeline.graphicsPipeline);


        skyboxBaseModel.populateCommandBuffer(&commandBuffer, currentImage, 0);
    }

    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        static float lastTime = 0.0f;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>
                (currentTime - startTime).count();
        float deltaT = time - lastTime;
        lastTime = time;

        const float farPlane = 50.0;

        bool isAtLeastOneKeyPressed = false;

        keys_status[GLFW_KEY_A] = glfwGetKey(window, GLFW_KEY_A);
        keys_status[GLFW_KEY_S] = glfwGetKey(window, GLFW_KEY_S);
        keys_status[GLFW_KEY_D] = glfwGetKey(window, GLFW_KEY_D);
        keys_status[GLFW_KEY_F] = glfwGetKey(window, GLFW_KEY_F);
        keys_status[GLFW_KEY_W] = glfwGetKey(window, GLFW_KEY_W);
        keys_status[GLFW_KEY_UP] = glfwGetKey(window, GLFW_KEY_UP);
        keys_status[GLFW_KEY_DOWN] = glfwGetKey(window, GLFW_KEY_DOWN);
        keys_status[GLFW_KEY_RIGHT] = glfwGetKey(window, GLFW_KEY_RIGHT);
        keys_status[GLFW_KEY_LEFT] = glfwGetKey(window, GLFW_KEY_LEFT);

        if (keys_status[GLFW_KEY_A] == GLFW_PRESS) {
            drone.move(DroneDirections::L, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_A] == GLFW_RELEASE) {
            drone.stop(DroneDirections::L, deltaT);
        }

        if (keys_status[GLFW_KEY_S] == GLFW_PRESS) {
            drone.move(DroneDirections::B, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_S] == GLFW_RELEASE) {
            drone.stop(DroneDirections::B, deltaT);
        }

        if (keys_status[GLFW_KEY_D] == GLFW_PRESS) {
            drone.move(DroneDirections::R, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_D] == GLFW_RELEASE) {
            drone.stop(DroneDirections::R, deltaT);
        }

        if (keys_status[GLFW_KEY_W] == GLFW_PRESS) {
            drone.move(DroneDirections::F, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_W] == GLFW_RELEASE) {
            drone.stop(DroneDirections::F, deltaT);
        }

        if (keys_status[GLFW_KEY_UP] == GLFW_PRESS) {
            drone.move(DroneDirections::U, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_UP] == GLFW_RELEASE) {
            drone.stop(DroneDirections::U, deltaT);
        }

        if (keys_status[GLFW_KEY_DOWN] == GLFW_PRESS) {
            drone.move(DroneDirections::D, deltaT);
            isAtLeastOneKeyPressed = true;
        } else if (keys_status[GLFW_KEY_DOWN] == GLFW_RELEASE) {
            drone.stop(DroneDirections::D, deltaT);
        }

        if (keys_status[GLFW_KEY_RIGHT] == GLFW_PRESS) {
            drone.moveView(deltaT, -1);
            isAtLeastOneKeyPressed = true;
        }

        if (keys_status[GLFW_KEY_LEFT] == GLFW_PRESS) {
            drone.moveView(deltaT, 1);
            isAtLeastOneKeyPressed = true;
        }

        isAtLeastOneKeyPressed ? drone.activateFans() : drone.deactivateFans();

        glm::mat4 cameraMatrix = glm::lookAt(cameraPosition, drone.position, glm::vec3(0, 1, 0));

        GlobalUniformBufferObject gubo{};
        gubo.view = cameraMatrix;
        gubo.proj = glm::perspective(glm::radians(60.0f),
                                     swapChainExtent.width / (float) swapChainExtent.height,
                                     0.1f, farPlane);
        gubo.proj[1][1] *= -1;

        UniformBufferObject ubo{};
        void *data;

        vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

        // Terrain
        terrain.draw(currentImage, &ubo, &data, &device);

        // Drone
        drone.draw(currentImage, &ubo, &data, &device);

        // Skybox
        SkyBoxUniformBufferObject subo{};

        void *dataSB;
        subo.view = cameraMatrix;
        subo.proj = glm::perspective(glm::radians(60.0f),
                                     swapChainExtent.width / (float) swapChainExtent.height,
                                     0.1f, 8 * farPlane);

        glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), drone.position) *
                                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -farPlane / 2, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(4 * farPlane));

        subo.proj[1][1] *= -1;
        skyboxBaseModel.draw(currentImage, &subo, &dataSB, &device, worldMatrix);
    }

};

// This is the main: probably you do not need to touch this!
int main() {
    DroneSimulator app;

    try {
        app.run();
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}