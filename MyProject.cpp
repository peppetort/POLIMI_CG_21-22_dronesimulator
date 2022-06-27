// This has been adapted from the Vulkan tutorial

#include "Models.hpp"

// The uniform buffer object used in this example
struct GlobalUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

//struct UniformBufferObject {
//	alignas(16) glm::mat4 model;
//};

struct SkyBoxUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 model;
};


// MAIN !
class MyProject : public BaseProject {
private:
    glm::vec3 cameraAngle = glm::vec3(0.0f);
    glm::vec3 cameraPosition = glm::vec3(0.0f, 1.6f, 3.0f);

protected:
    // Here you list all the Vulkan objects you need:

    // Descriptor Layouts [what will be passed to the shaders]
    DescriptorSetLayout DSLglobal;
    DescriptorSetLayout DSLobj;

    // Pipelines [Shader couples]
    Pipeline P1;

    DescriptorSet DS_global;

    //Terrain
    BaseModel terrainBaseModel = BaseModel(this, &DSLobj);

    // Drone
    BaseModel droneBaseModel = BaseModel(this, &DSLobj);
    //Fans
    BaseModel fansArray[4] = {BaseModel(this, &DSLobj), BaseModel(this, &DSLobj), BaseModel(this, &DSLobj),
                              BaseModel(this, &DSLobj)};

    Drone drone = Drone(&droneBaseModel, fansArray);


    /*---- SKYBOX ----*/

    DescriptorSetLayout SkyBoxDescriptorSetLayout; // for skybox
    Pipeline SkyBoxPipeline;        // for skybox
    Model M_SkyBox;
    Texture T_SkyBox;
    DescriptorSet DS_SkyBox;    // instance SkyBoxDescriptorSetLayout

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
        P1.init(this, "../shaders/vert.spv", "../shaders/frag.spv", {&DSLglobal, &DSLobj});

        // Terrain
/*        M_Terrain.init(this, "../models/Terrain.obj");
        T_Terrain.init(this, "../textures/PaloDuroPark.jpg");
        DS_Terrain.init(this, &DSLobj, {
                // the second parameter, is a pointer to the Uniform Set Layout of this set
                // the last parameter is an array, with one element per binding of the set.
                // first  elmenet : the binding number
                // second element : UNIFORM or TEXTURE (an enum) depending on the type
                // third  element : only for UNIFORMs, the size of the corresponding C++ object
                // fourth element : only for TEXTUREs, the pointer to the corresponding texture object
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0,                           &T_Terrain}
        });*/
        terrainBaseModel.init("../models/Terrain.obj", "../textures/PaloDuroPark.jpg");


        // Drone

        droneBaseModel.init("../models/droneFixed.obj", "../textures/White.png");
        //fanBaseModel.init("../models/fan.obj");

//        M_Drone.init(this, "../models/droneFixed.obj");
//        T_Drone.init(this, "../textures/White.png");
//        DS_Drone.init(this, &DSLobj, {
//                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
//                {1, TEXTURE, 0,                           &T_Drone}
//        });
//
//        // Fans
        for (auto &i: fansArray) {
            i.init("../models/fan.obj");
        }

//
//        M_Fan.init(this, "../models/fan.obj");
//        DS_Fan1.init(this, &DSLobj, {
//                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
//                {1, TEXTURE, 0,                           &T_Drone}
//        });
//        DS_Fan2.init(this, &DSLobj, {
//                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
//                {1, TEXTURE, 0,                           &T_Drone}
//        });
//        DS_Fan3.init(this, &DSLobj, {
//                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
//                {1, TEXTURE, 0,                           &T_Drone}
//        });
//        DS_Fan4.init(this, &DSLobj, {
//                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
//                {1, TEXTURE, 0,                           &T_Drone}
//        });

        /*---- SKYBOX ----*/

        SkyBoxDescriptorSetLayout.init(this, {
                // this array contains the binding:
                // first  element : the binding number
                // second element : the time of element (buffer or texture)
                // third  element : the pipeline stage where it will be used
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
        });

        SkyBoxPipeline.init(this, "../shaders/SkyBoxvert.spv", "../shaders/SkyBoxfrag.spv",
                            {&SkyBoxDescriptorSetLayout});


        M_SkyBox.init(this, "../models/SkyBoxCube.obj");
        T_SkyBox.init(this, "../textures/skybox.png");
        DS_SkyBox.init(this, &SkyBoxDescriptorSetLayout, {
                {0, UNIFORM, sizeof(SkyBoxUniformBufferObject), nullptr},
                {1, TEXTURE, 0,                                 &T_SkyBox}
        });
    }

    // Here you destroy all the objects you created!
    void localCleanup() {
/*        DS_Terrain.cleanup();
        T_Terrain.cleanup();
        M_Terrain.cleanup();*/
        terrainBaseModel.cleanUp();

//        DS_Drone.cleanup();
//        T_Drone.cleanup();
//        M_Drone.cleanup();
//
//        DS_Fan1.cleanup();
//        DS_Fan2.cleanup();
//        DS_Fan3.cleanup();
//        DS_Fan4.cleanup();
//        M_Fan.cleanup();

        droneBaseModel.cleanUp();
        for (auto &i: fansArray) {
            i.cleanUp();
        }

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

        // Bind P1
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          P1.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                0, nullptr);

        // Terrain
        VkBuffer vertexBuffers[] = {terrainBaseModel.model.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, terrainBaseModel.model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                P1.pipelineLayout, 1, 1, &terrainBaseModel.descriptorSet.descriptorSets[currentImage],
                                0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(terrainBaseModel.model.indices.size()), 1, 0, 0, 0);


        // Drone
        VkBuffer vertexBuffers4[] = {droneBaseModel.model.vertexBuffer};
        VkDeviceSize offsets4[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers4, offsets4);
        vkCmdBindIndexBuffer(commandBuffer, droneBaseModel.model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                P1.pipelineLayout, 1, 1, &droneBaseModel.descriptorSet.descriptorSets[currentImage],
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
                                    P1.pipelineLayout, 1, 1, &fanBaseModel.descriptorSet.descriptorSets[currentImage],
                                    0, nullptr);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(fanBaseModel.model.indices.size()), 1, 0, 0, 0);
        }


//        vkCmdBindDescriptorSets(commandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                P1.pipelineLayout, 1, 1, &DS_Fan2.descriptorSets[currentImage],
//                                0, nullptr);
//        vkCmdDrawIndexed(commandBuffer,
//                         static_cast<uint32_t>(M_Fan.indices.size()), 1, 0, 0, 0);
//
//        vkCmdBindDescriptorSets(commandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                P1.pipelineLayout, 1, 1, &DS_Fan3.descriptorSets[currentImage],
//                                0, nullptr);
//        vkCmdDrawIndexed(commandBuffer,
//                         static_cast<uint32_t>(M_Fan.indices.size()), 1, 0, 0, 0);
//
//        vkCmdBindDescriptorSets(commandBuffer,
//                                VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                P1.pipelineLayout, 1, 1, &DS_Fan4.descriptorSets[currentImage],
//                                0, nullptr);
//        vkCmdDrawIndexed(commandBuffer,
//                         static_cast<uint32_t>(M_Fan.indices.size()), 1, 0, 0, 0);



        //Pipeline for skybox
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          SkyBoxPipeline.graphicsPipeline);

        VkBuffer vertexBuffers2[] = {M_SkyBox.vertexBuffer};
        VkDeviceSize offsets2[] = {0};
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
        static float lastTime = 0.0f;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>
                (currentTime - startTime).count();
        float deltaT = time - lastTime;
        lastTime = time;
        const float ROT_SPEED = glm::radians(60.0f);

        bool keyPressed = false;

        if (glfwGetKey(window, GLFW_KEY_LEFT)) {
            cameraAngle.y += deltaT * ROT_SPEED;
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
            cameraAngle.y -= deltaT * ROT_SPEED;
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_UP)) {
            drone.moveUp(deltaT);
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN)) {
            drone.moveDown(deltaT);
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_A)) {
            drone.moveLeft(deltaT, cameraAngle.y);
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            drone.moveRight(deltaT, cameraAngle.y);
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            drone.moveBackward(deltaT, cameraAngle.y);
            keyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_W)) {
            drone.moveForward(deltaT, cameraAngle.y);
            keyPressed = true;
        }

        if (!keyPressed) {
            drone.fall();
        }


        glm::mat4 cameraMatrix = glm::lookAt(cameraPosition, drone.position, glm::vec3(0, 1, 0));


        glm::mat3 CamDir = glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
                           glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.z, glm::vec3(1.0f, 0.0f, 0.0f))) *
                           glm::mat3(glm::rotate(glm::mat4(1.0f), cameraAngle.x, glm::vec3(0.0f, 0.0f, 1.0f)));

        GlobalUniformBufferObject gubo{};
        gubo.view = cameraMatrix;
        gubo.proj = glm::perspective(glm::radians(45.0f),
                                     swapChainExtent.width / (float) swapChainExtent.height,
                                     0.1f, 50.0f);
        gubo.proj[1][1] *= -1;

        UniformBufferObject ubo{};
        void *data;

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
        vkMapMemory(device, terrainBaseModel.descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, terrainBaseModel.descriptorSet.uniformBuffersMemory[0][currentImage]);

        // Draw drone (with fans)
        drone.draw(currentImage, &ubo, &data, &device);

        /*---- SKYBOX ----*/

        SkyBoxUniformBufferObject subo{};

        void *dataSB;

        subo.model = glm::mat4(1.0f);
        subo.view = glm::transpose(glm::mat4(CamDir));
        subo.proj = glm::perspective(glm::radians(45.0f),
                                     swapChainExtent.width / (float) swapChainExtent.height,
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
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}