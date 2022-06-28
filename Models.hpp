#include "DroneSimulator.hpp"
#include <string>
#include <utility>
#include <glm/gtc/quaternion.hpp>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
};

struct KeyStatus {
    int actualStatus;
    int lastStatus;
};

static auto startTime = std::chrono::high_resolution_clock::now();


class BaseModel {
public:
    Model model;
    Texture texture;
    DescriptorSet descriptorSet;
    BaseProject *baseProjectPtr;
    DescriptorSetLayout *descriptorSetLayoutPtr;

    BaseModel(BaseProject *baseProjectPtr, DescriptorSetLayout *descriptorSetLayoutPtr) {
        this->baseProjectPtr = baseProjectPtr;
        this->descriptorSetLayoutPtr = descriptorSetLayoutPtr;
    }

    void init(std::string modelPath, std::string texturePath) {
        model.init(baseProjectPtr, std::move(modelPath));
        texture.init(baseProjectPtr, std::move(texturePath));
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

    void cleanUp() {
        model.cleanup();
        texture.cleanup();
        descriptorSet.cleanup();
    }

    void cleanUpNoTexture() {
        model.cleanup();
        descriptorSet.cleanup();
    }

};

class Drone {
private:
    const float MOVE_SPEED = 10.f;
    const float FAN_SPEED = 1000.f;
    const float INCLINATION_SPEED = 1.f;
    const float MAX_INCLINATION = glm::radians(10.f);
    bool areFansActive = false;

public:
    BaseModel *droneBaseModel;
    BaseModel *fanBaseModelList;

    glm::mat4 worldMatrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat direction = glm::quat(glm::vec3(0.f, 0.f, 0.f));
    float scale_factor = 0.3f;

    Drone(BaseModel *droneBaseModelPtr, BaseModel fanBaseModelList[4]) {
        this->droneBaseModel = droneBaseModelPtr;
        this->fanBaseModelList = fanBaseModelList;
    }

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {
        glm::mat4 droneTranslation = glm::translate(glm::mat4(1), position);
        glm::mat4 droneRotation = glm::mat4(direction);
        glm::mat4 droneScaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
        worldMatrix = droneTranslation * droneRotation * droneScaling;
        (*uboPtr).model = worldMatrix;

        vkMapMemory(*devicePtr, (*droneBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(*uboPtr), 0, &dataPtr);
        memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
        vkUnmapMemory(*devicePtr, (*droneBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage]);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>
                (currentTime - startTime).count();

        //Drawing fans
        glm::mat4 fansActiveRotation = areFansActive ? glm::rotate(glm::mat4(1.0f),
                                                                   time * FAN_SPEED * glm::radians(90.0f),
                                                                   glm::vec3(0.0f, 1.0f, 0.0f)) : glm::mat4(1.f);
        glm::mat4 inclinationRotation = glm::mat4(direction);

        glm::mat4 fanRotation = inclinationRotation * fansActiveRotation;
        glm::mat4 fanScaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor * 0.68f));

        for (int i = 0; i < 4; i++) {
            glm::mat4 fanTranslationWithDrone = glm::translate(glm::mat4(1.f), position);
            glm::mat4 fanTranslationWRTDroneCenter;
            switch (i) {
                case 0:
                    fanTranslationWRTDroneCenter = glm::translate(glm::mat4(1.0f), glm::vec3(-0.33f, 0.1f, -0.33f));
                    break;
                case 1:
                    fanTranslationWRTDroneCenter = glm::translate(glm::mat4(1.0f), glm::vec3(0.33f, 0.1f, 0.33f));
                    break;
                case 2:
                    fanTranslationWRTDroneCenter = glm::translate(glm::mat4(1.0f), glm::vec3(-0.33f, 0.1f, 0.33f));
                    break;
                case 3:
                    fanTranslationWRTDroneCenter = glm::translate(glm::mat4(1.0f), glm::vec3(0.33f, 0.1f, -0.33f));
                    break;
                default:
                    fanTranslationWRTDroneCenter = glm::mat4(1.f);
            }

            (*uboPtr).model = fanTranslationWithDrone * fanRotation * fanTranslationWRTDroneCenter * fanScaling;

            vkMapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                        sizeof(*uboPtr), 0, &dataPtr);
            memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
            vkUnmapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage]);
        }
    }

    void onMoveUp(float deltaT) {
        position.y += deltaT * MOVE_SPEED;
        areFansActive = true;
    }

    void onMoveUpRelease() {
        //TODO: gravity effect
    }

    void onMoveForward(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
        direction.x -= abs(direction.x) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
        areFansActive = true;
    }

    void onMoveForwardRelease() {
        direction.x = 0.f;
    }

    void onMoveBackward(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
        direction.x += abs(direction.x) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
        areFansActive = true;
    }

    void onMoveBackwardRelease() {
        direction.x = 0.f;
    }

    void onMoveLeft(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
        direction.z += abs(direction.z) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
        areFansActive = true;
    }

    void onMoveLeftRelease() {
        direction.z = 0.f;
    }

    void onMoveRight(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
        direction.z -= abs(direction.z) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
        areFansActive = true;
    }

    void onMoveRightRelease() {
        direction.z = 0.f;
    }

    void deactivateFans() {
        areFansActive = false;
    }


};