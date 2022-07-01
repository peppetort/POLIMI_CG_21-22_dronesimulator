#include "DroneSimulator.hpp"
#include <string>
#include <utility>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/string_cast.hpp>
#include <map>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
};
struct SkyBoxUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 model;
};


class BaseModel {
public:
    Model model;
    Texture texture{};
    DescriptorSet descriptorSet;

    BaseProject *baseProjectPtr;
    DescriptorSetLayout *descriptorSetLayoutPtr;
    Pipeline *pipeline;

    BaseModel(BaseProject *baseProjectPtr, DescriptorSetLayout *descriptorSetLayoutPtr, Pipeline *pipeline) {
        this->baseProjectPtr = baseProjectPtr;
        this->descriptorSetLayoutPtr = descriptorSetLayoutPtr;
        this->pipeline = pipeline;
    }

    void init(std::string modelPath, std::string texturePath, bool isSkyBox = false) {
        model.init(baseProjectPtr, std::move(modelPath));
        texture.init(baseProjectPtr, std::move(texturePath));
        if (isSkyBox)
            descriptorSet.init(baseProjectPtr, descriptorSetLayoutPtr, {
                    {0, UNIFORM, sizeof(SkyBoxUniformBufferObject), nullptr},
                    {1, TEXTURE, 0,                                 &texture}
            });
        else
            descriptorSet.init(baseProjectPtr, descriptorSetLayoutPtr, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0,                           &texture}
            });
    }

    void init(std::string modelPath) {
        model.init(baseProjectPtr, std::move(modelPath));
        descriptorSet.init(baseProjectPtr, descriptorSetLayoutPtr, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr}
        });
    }

    void populateCommandBuffer(VkCommandBuffer *commandBuffer, int currentImage) {
        VkBuffer vertexBuffers[] = {model.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(*commandBuffer, model.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(*commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                (*pipeline).pipelineLayout, 1, 1, &descriptorSet.descriptorSets[currentImage],
                                0, nullptr);
        vkCmdDrawIndexed(*commandBuffer,
                         static_cast<uint32_t>(model.indices.size()), 1, 0, 0, 0);
    }

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr,
              glm::mat4 worldMatrix) {
        (*uboPtr).model = worldMatrix;
        vkMapMemory(*devicePtr, descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(*uboPtr), 0, &dataPtr);
        memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
        vkUnmapMemory(*devicePtr, descriptorSet.uniformBuffersMemory[0][currentImage]);
    }


    void cleanUp() {
        model.cleanup();
        if (descriptorSet.uniformBuffers.size() > 1) {
            texture.cleanup();
        }
        descriptorSet.cleanup();
    }

};

enum DroneDirections {
    F, B, R, L, U, D
};

class Drone {

private:

    // configurable parameters
    const glm::vec3 INITIAL_POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
    const glm::vec3 INITIAL_DIRECTION = glm::vec3(0.0f, 0.0f, 0.0f);

    const float SCALE_FACTOR = 0.015f;

    const float FAN_MAX_SPEED = 30.f;
    const float FAN_MIN_SPEED = 10.f;
    const float FAN_DECELERATION_RATE = 0.5f;
    const float FAN_ACCELERATION_RATE = 0.5f;

    const float MIN_FAN_SPEED_TO_MOVE = 18.f;

    const float DRONE_MAX_SPEED = 15.f;

    const float DRONE_ACCELERATION_RATE = 0.5f;
    const float DRONE_DECELERATION_RATE = 0.1f;

    const float INCLINATION_SPEED = glm::radians(45.f);
    const float MAX_INCLINATION = glm::radians(15.f);

    const float ROTATION_SPEED = glm::radians(35.f);

    // internal variables
    float fanSpeed = FAN_MIN_SPEED;
    glm::quat fanRotation = glm::quat(glm::vec3(0));

    std::map<DroneDirections, glm::vec3> directionToVectorMap = {
            {DroneDirections::F, glm::vec3(0, 0, -1)},
            {DroneDirections::B, glm::vec3(0, 0, 1)},
            {DroneDirections::R, glm::vec3(1, 0, 0)},
            {DroneDirections::L, glm::vec3(-1, 0, 0)},
            {DroneDirections::U, glm::vec3(0, 1, 0)},
            {DroneDirections::D, glm::vec3(0, -1, 0)},
    };

    std::map<DroneDirections, float> droneSpeedPerDirectionMap = {
            {DroneDirections::F, 0.f},
            {DroneDirections::B, 0.f},
            {DroneDirections::R, 0.f},
            {DroneDirections::L, 0.f},
            {DroneDirections::U, 0.f},
            {DroneDirections::D, 0.f},
    };

    std::map<DroneDirections, glm::vec3> directionToInclinationVectorMap = {
            {DroneDirections::F, glm::vec3(-1, 0, 0)},
            {DroneDirections::B, glm::vec3(1, 0, 0)},
            {DroneDirections::R, glm::vec3(0, 0, -1)},
            {DroneDirections::L, glm::vec3(0, 0, 1)},
    };

    void updateDroneAndCameraPosition(float deltaT, glm::vec3 moveDirection, float speed) {
        // move drone
        position += speed * glm::vec3(glm::mat4(lookDirection) * glm::vec4(moveDirection, 1)) * deltaT;
        // move camera
        *cameraPosition += speed * glm::vec3(glm::mat4(lookDirection) * glm::vec4(moveDirection, 1)) * deltaT;
    }

public:

    BaseModel droneBaseModel;
    BaseModel fanBaseModelList[4];

    glm::vec3 position = INITIAL_POSITION;
    glm::quat direction = glm::quat(INITIAL_DIRECTION);
    glm::quat lookDirection = glm::quat(glm::vec3(0.f));

    glm::vec3 *cameraPosition = nullptr;

    Drone(BaseProject *baseProjectPtr, DescriptorSetLayout *descriptorSetLayoutPtr,
          Pipeline *pipeline) : droneBaseModel(baseProjectPtr, descriptorSetLayoutPtr, pipeline),
                                fanBaseModelList{BaseModel(baseProjectPtr, descriptorSetLayoutPtr, pipeline),
                                                 BaseModel(baseProjectPtr, descriptorSetLayoutPtr, pipeline),
                                                 BaseModel(baseProjectPtr, descriptorSetLayoutPtr, pipeline),
                                                 BaseModel(baseProjectPtr, descriptorSetLayoutPtr, pipeline)} {};

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {

        glm::mat4 droneTranslation = glm::translate(glm::mat4(1), position);
        glm::mat4 droneRotation = glm::mat4(lookDirection) * glm::mat4(direction);
        glm::mat4 droneScaling = glm::scale(glm::mat4(1.0f), glm::vec3(SCALE_FACTOR));
        glm::mat4 droneWorldMatrix = droneTranslation * droneRotation * droneScaling;
        droneBaseModel.draw(currentImage, uboPtr, dataPtr, devicePtr, droneWorldMatrix);


        //Drawing fans
        fanRotation = glm::rotate(fanRotation, -glm::radians(fanSpeed), glm::vec3(0, 1, 0));

        glm::mat4 fanScaling = glm::scale(glm::mat4(1.0f), glm::vec3(SCALE_FACTOR));
        glm::mat4 fanTranslationWithDrone = glm::translate(glm::mat4(1.f), position);
        glm::mat4 movesAndInclination = fanTranslationWithDrone * droneRotation;
        glm::mat4 scalingAndRotation = fanScaling * glm::mat4(fanRotation);

        glm::mat4 positioningWRTDrone = glm::translate(glm::mat4(1.0f), glm::vec3(0.54f, 0.26f, -0.4f));
        glm::mat4 fanWorldMatrix = movesAndInclination * positioningWRTDrone * scalingAndRotation;
        fanBaseModelList[0].draw(currentImage, uboPtr, dataPtr, devicePtr, fanWorldMatrix);

        positioningWRTDrone = glm::translate(glm::mat4(1.0f), glm::vec3(-0.54f, 0.26f, -0.4f));
        fanWorldMatrix = movesAndInclination * positioningWRTDrone * scalingAndRotation;
        fanBaseModelList[1].draw(currentImage, uboPtr, dataPtr, devicePtr, fanWorldMatrix);

        positioningWRTDrone = glm::translate(glm::mat4(1.0f), glm::vec3(-0.54f, 0.11f, 0.4f));
        fanWorldMatrix = movesAndInclination * positioningWRTDrone * scalingAndRotation;
        fanBaseModelList[2].draw(currentImage, uboPtr, dataPtr, devicePtr, fanWorldMatrix);

        positioningWRTDrone = glm::translate(glm::mat4(1.0f), glm::vec3(0.54f, 0.11f, 0.4f));
        fanWorldMatrix = movesAndInclination * positioningWRTDrone * scalingAndRotation;
        fanBaseModelList[3].draw(currentImage, uboPtr, dataPtr, devicePtr, fanWorldMatrix);
    }

    void setCameraPosition(glm::vec3 *cameraPosition) {
        this->cameraPosition = cameraPosition;
    }

    void move(DroneDirections droneDirection, float deltaT) {
        if (fanSpeed < MIN_FAN_SPEED_TO_MOVE * 0.5) {
            return;
        }

        auto x = directionToInclinationVectorMap.find(droneDirection);

        if (x != directionToInclinationVectorMap.end()) {
            float inclination_rate = glm::dot(glm::eulerAngles(direction),
                                              directionToInclinationVectorMap[droneDirection]);

            if (inclination_rate < MAX_INCLINATION) {
                direction = glm::rotate(direction, deltaT * INCLINATION_SPEED,
                                        directionToInclinationVectorMap[droneDirection]);
            } else if (inclination_rate > MAX_INCLINATION) {
                direction = glm::rotate(direction, glm::radians(0.f),
                                        directionToInclinationVectorMap[droneDirection]);
            }
        }


        if (fanSpeed < MIN_FAN_SPEED_TO_MOVE) {
            return;
        }

        float speed = droneSpeedPerDirectionMap[droneDirection];
        //std::cout << speed << std::endl;
        speed += speed < DRONE_MAX_SPEED ? DRONE_ACCELERATION_RATE : 0.f;
        droneSpeedPerDirectionMap[droneDirection] = speed;
        updateDroneAndCameraPosition(deltaT, directionToVectorMap[droneDirection], speed);
    }

    void stop(DroneDirections droneDirection, float deltaT) {
        auto x = directionToInclinationVectorMap.find(droneDirection);

        if (x != directionToInclinationVectorMap.end()) {
            float inclination_rate = glm::dot(glm::eulerAngles(direction),
                                              directionToInclinationVectorMap[droneDirection]);

            if (inclination_rate > 0.f) {
                direction = glm::rotate(direction, -deltaT * INCLINATION_SPEED,
                                        directionToInclinationVectorMap[droneDirection]);
            } else if (inclination_rate < 0.f) {
                direction = glm::rotate(direction, glm::radians(0.f),
                                        directionToInclinationVectorMap[droneDirection]);
            }
        }

        float speed = droneSpeedPerDirectionMap[droneDirection];
        if (speed <= 0) {
            return;
        }
        // std::cout << speed << std::endl;
        speed -= speed > 0.f ? DRONE_DECELERATION_RATE : 0.f;
        if (speed < 0.f) {
            speed = 0.f;
        }
        droneSpeedPerDirectionMap[droneDirection] = speed;
        updateDroneAndCameraPosition(deltaT, directionToVectorMap[droneDirection], speed);
    }


    void activateFans() {
        if (fanSpeed >= FAN_MAX_SPEED) {
            return;
        }
        fanSpeed += fanSpeed < FAN_MAX_SPEED ? FAN_ACCELERATION_RATE : 0.f;
        std::cout << "ACTIVATE: " << fanSpeed << std::endl;
    }

    void deactivateFans() {
        if (fanSpeed == FAN_MIN_SPEED) {
            return;
        }
        fanSpeed -= fanSpeed > FAN_MIN_SPEED ? FAN_DECELERATION_RATE : 0.f;
        if (fanSpeed < FAN_MIN_SPEED) {
            fanSpeed = FAN_MIN_SPEED;
        }
        std::cout << "DEACTIVATE: " << fanSpeed << std::endl;
    }

    void moveView(float deltaT, float v) {
        lookDirection = glm::rotate(lookDirection, v * deltaT * ROTATION_SPEED, glm::vec3(0, 1, 0));
        glm::mat4 translation = glm::translate(glm::mat4(1.f), position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), v * ROTATION_SPEED * deltaT, glm::vec3(0, 1, 0));
        (*cameraPosition) = translation * rotation * glm::inverse(translation) * glm::vec4((*cameraPosition), 1.0f);
    }
};

class Terrain {
public:
    BaseModel *terrainBaseModel;

    glm::vec3 position = glm::vec3(-20.0f, -10.0f, 30.0f);
    glm::vec3 direction = glm::vec3(90.f, 0.f, 0.f);
    float scale_factor = 5.f;

    Terrain(BaseModel *terrainBaseModel) {
        this->terrainBaseModel = terrainBaseModel;
    }

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {

        glm::mat4 translation = glm::translate(glm::mat4(1), position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
                                         glm::radians(-90.0f),
                                         glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));

        glm::mat4 worldMatrix = translation * rotation * scaling;

        (*uboPtr).model = worldMatrix;
        vkMapMemory(*devicePtr, (*terrainBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(*uboPtr), 0, &dataPtr);
        memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
        vkUnmapMemory(*devicePtr, (*terrainBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage]);
    }
};