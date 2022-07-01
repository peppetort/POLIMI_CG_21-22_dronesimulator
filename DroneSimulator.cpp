// This has been adapted from the Vulkan tutorial

#include "Models.hpp"

// The uniform buffer object used in this example
struct GlobalUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


// MAIN !
class DroneSimulator : public BaseProject {


private:
    glm::vec3 cameraAngle = glm::vec3(0.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f, 1.6f, 3.0f);
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

    // Pipelines [Shader couples]
    Pipeline PipelineTerrain;

    DescriptorSet DS_global;

    //Terrain
    BaseModel terrainBaseModel = BaseModel(this, &DSLobj, &PipelineTerrain);
    Terrain terrain = Terrain(&terrainBaseModel);

    Pipeline PipelineDrone;
    // Drone
    BaseModel droneBaseModel = BaseModel(this, &DSLobj, &PipelineDrone);
    //Fans
    BaseModel fansArray[4] = {BaseModel(this, &DSLobj, &PipelineDrone), BaseModel(this, &DSLobj, &PipelineDrone),
                              BaseModel(this, &DSLobj, &PipelineDrone),
                              BaseModel(this, &DSLobj, &PipelineDrone)};

    Drone drone = Drone(&droneBaseModel, fansArray);

    /*---- SKYBOX ----*/

    DescriptorSetLayout SkyBoxDescriptorSetLayout; // for skybox
    Pipeline SkyBoxPipeline;        // for skybox
    BaseModel skybox = BaseModel(this, &SkyBoxDescriptorSetLayout, &SkyBoxPipeline);

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, title and initial background
        windowWidth = 800;
        windowHeight = 600;
        windowTitle = "Drone simulator";
        initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};

        // Descriptor pool sizes
        uniformBlocksInPool = 20;
        texturesInPool = 20;
        setsInPool = 20;
    }

    // Here you load and setup all your Vulkan objects
    void localInit() {
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
        // be used in this pipeline. The first element will be set 0, and so on..
        PipelineTerrain.init(this, "shaders/shaderTerrainVert.spv", "shaders/shaderTerrainFrag.spv", {&DSLglobal, &DSLobj},false);

        // Terrain
        terrainBaseModel.init("../models/Terrain.obj", "../textures/cliff1Color.png");

        PipelineDrone.init(this, "shaders/shaderDroneVert.spv", "shaders/shaderDroneFrag.spv", { &DSLglobal, &DSLobj }, false);

        // Drone
        droneBaseModel.init("../models/test.obj", "../textures/drone.png");
        // Fans
        for (auto &i: fansArray) {
            i.init("../models/test2.obj");
        }

        /*---- SKYBOX ----*/
        SkyBoxDescriptorSetLayout.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

        SkyBoxPipeline.init(this, "shaders/shaderSkyBoxVert.spv", "shaders/shaderSkyBoxFrag.spv",
                            {&SkyBoxDescriptorSetLayout},true);
        skybox.init("models/SkyBox.obj", "textures/fog.png", true);

    }

    // Here you destroy all the objects you created!
    void localCleanup() {
        terrainBaseModel.cleanUp();

        droneBaseModel.cleanUp();

        for (auto &i: fansArray) {
            i.cleanUp();
        }

        DS_global.cleanup();

        PipelineTerrain.cleanup();
        PipelineDrone.cleanup();
        DSLglobal.cleanup();
        DSLobj.cleanup();

        skybox.cleanUp();
        SkyBoxPipeline.cleanup();
        SkyBoxDescriptorSetLayout.cleanup();
    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

        // Bind PipelineTerrain
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineTerrain.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineTerrain.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                0, nullptr);

        // Terrain
        VkBuffer vertexBuffers[] = {terrainBaseModel.model.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, terrainBaseModel.model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineTerrain.pipelineLayout, 1, 1, &terrainBaseModel.descriptorSet.descriptorSets[currentImage],
                                0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(terrainBaseModel.model.indices.size()), 1, 0, 0, 0);

        // Bind PipelineDrone

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineDrone.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineDrone.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
            0, nullptr);
        // Drone
        VkBuffer vertexBuffers4[] = {droneBaseModel.model.vertexBuffer};
        VkDeviceSize offsets4[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers4, offsets4);
        vkCmdBindIndexBuffer(commandBuffer, droneBaseModel.model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
            PipelineDrone.pipelineLayout, 1, 1, &droneBaseModel.descriptorSet.descriptorSets[currentImage],
                                0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(droneBaseModel.model.indices.size()), 1, 0, 0, 0);

//        // Fans
        for (auto &fanBaseModel: fansArray) {
            VkBuffer vertexBuffers5[] = {fanBaseModel.model.vertexBuffer};
            VkDeviceSize offsets5[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers5, offsets5);
            vkCmdBindIndexBuffer(commandBuffer, fanBaseModel.model.indexBuffer, 0,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    PipelineDrone.pipelineLayout, 1, 1, &fanBaseModel.descriptorSet.descriptorSets[currentImage],
                                    0, nullptr);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(fanBaseModel.model.indices.size()), 1, 0, 0, 0);
        }
        


        //Pipeline for skybox
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          SkyBoxPipeline.graphicsPipeline);

        VkBuffer vertexBuffers2[] = {skybox.model.vertexBuffer};
        VkDeviceSize offsets2[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
        vkCmdBindIndexBuffer(commandBuffer, skybox.model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                SkyBoxPipeline.pipelineLayout, 0, 1, &skybox.descriptorSet.descriptorSets[currentImage],
                                0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(skybox.model.indices.size()), 1, 0, 0, 0);
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
        if (keys_status[GLFW_KEY_A] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_S] = glfwGetKey(window, GLFW_KEY_S);
        if (keys_status[GLFW_KEY_S] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_D] = glfwGetKey(window, GLFW_KEY_D);
        if (keys_status[GLFW_KEY_D] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_F] = glfwGetKey(window, GLFW_KEY_F);
        if (keys_status[GLFW_KEY_F] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_W] = glfwGetKey(window, GLFW_KEY_W);
        if (keys_status[GLFW_KEY_W] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_UP] = glfwGetKey(window, GLFW_KEY_UP);
        if (keys_status[GLFW_KEY_UP] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_DOWN] = glfwGetKey(window, GLFW_KEY_DOWN);
        if (keys_status[GLFW_KEY_DOWN] == GLFW_PRESS) {
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_RIGHT] = glfwGetKey(window, GLFW_KEY_RIGHT);
        if (keys_status[GLFW_KEY_RIGHT] == GLFW_PRESS) {
            drone.onLookRight(deltaT, &cameraPosition);
            isAtLeastOneKeyPressed = true;
        }
        keys_status[GLFW_KEY_LEFT] = glfwGetKey(window, GLFW_KEY_LEFT);
        if (keys_status[GLFW_KEY_LEFT] == GLFW_PRESS) {
            drone.onLookLeft(deltaT, &cameraPosition);
            isAtLeastOneKeyPressed = true;
        }

        drone.move(deltaT, glm::vec3(0, 0, -1), &cameraPosition, keys_status[GLFW_KEY_W]);
        drone.move(deltaT, glm::vec3(0, 0, 1), &cameraPosition, keys_status[GLFW_KEY_S]);
        drone.move(deltaT, glm::vec3(-1, 0, 0), &cameraPosition, keys_status[GLFW_KEY_A]);
        drone.move(deltaT, glm::vec3(1, 0, 0), &cameraPosition, keys_status[GLFW_KEY_D]);
        drone.move(deltaT, glm::vec3(0, 1, 0), &cameraPosition, keys_status[GLFW_KEY_UP]);
        drone.move(deltaT, glm::vec3(0, -1, 0), &cameraPosition, keys_status[GLFW_KEY_DOWN]);

        isAtLeastOneKeyPressed ? drone.activateFans() : drone.deactivateFans();

        glm::mat4 cameraMatrix = glm::lookAt(cameraPosition, drone.position, glm::vec3(0, 1, 0));


/*        glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
                           glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.z, glm::vec3(1.0f, 0.0f, 0.0f))) *
                           glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.x, glm::vec3(0.0f, 0.0f, 1.0f)));*/

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


        // For the Terrain
//        ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f)) *
//                    glm::translate(glm::mat4(1.0f), glm::vec3(-20.0f, -4.0f, 10.0f)) *
//                    glm::rotate(glm::mat4(1.0f),
//                                glm::radians(-90.0f),
//                                glm::vec3(1.0f, 0.0f, 0.0f));
        terrain.draw(currentImage, &ubo, &data, &device);

        // Draw drone (with fans)
        drone.draw(currentImage, &ubo, &data, &device);

        /*---- SKYBOX ----*/

        SkyBoxUniformBufferObject subo{};

        void *dataSB;

        /*glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.z, glm::vec3(1.0f, 0.0f, 0.0f))) *
            glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.x, glm::vec3(0.0f, 0.0f, 1.0f)));
        glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamDir)), -cameraPosition);*/

        //TODO  not working
        subo.model = glm::mat4(1.0f);/*glm::translate(glm::mat4(1.0f), -drone.position) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.2f));*/
        subo.view = cameraMatrix;
        subo.proj = glm::perspective(glm::radians(60.0f),
            swapChainExtent.width / (float)swapChainExtent.height,
            0.1f, farPlane);
        subo.proj[1][1] *= -1;
        // For the SkyBox
        vkMapMemory(device, skybox.descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(subo), 0, &dataSB);
        memcpy(dataSB, &subo, sizeof(subo));
        vkUnmapMemory(device, skybox.descriptorSet.uniformBuffersMemory[0][currentImage]);

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