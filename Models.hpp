#include "DroneSimulator.hpp"
#include <string>
#include <utility>
#include <glm/gtc/quaternion.hpp>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
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

};

class Drone {
private:
    const float MOVE_SPEED = 10.f;
    const float FAN_SPEED = 1000.f;
    bool areFansActive = false;

public:
    BaseModel *droneBaseModel;
    BaseModel *fanBaseModelList;

    glm::mat4 worldMatrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat direction = glm::quat(1, 0, 0, 0);
    float scale_factor = 0.3f;

    Drone(BaseModel *droneBaseModelPtr, BaseModel fanBaseModelList[4]) {
        this->droneBaseModel = droneBaseModelPtr;
        this->fanBaseModelList = fanBaseModelList;
    }

    Drone(BaseModel *droneBaseModelPtr) {
        this->droneBaseModel = droneBaseModelPtr;
    }

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {
        glm::mat4 droneTranslation = glm::translate(glm::mat4(1), position);
        auto droneRotation = glm::mat4(direction);
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
        glm::mat4 fanRotation = areFansActive ? glm::rotate(glm::mat4(1.0f), time * FAN_SPEED * glm::radians(90.0f),
                                                            glm::vec3(0.0f, 1.0f, 0.0f)) : glm::mat4(1.f) *
                                                                                           glm::rotate(glm::mat4(1.0f),
                                                                                                       glm::radians(
                                                                                                               180.0f),
                                                                                                       glm::vec3(0.0f,
                                                                                                                 0.0f,
                                                                                                                 1.0f));
        glm::mat4 fanScaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor * 0.68f));
        for (int i = 0; i < 4; i++) {

            glm::mat4 fanTranslation;
            switch (i) {
                case 0:
                    fanTranslation = glm::translate(glm::mat4(1.0f), position + glm::vec3(-0.33f, 0.13f, -0.33f));
                    break;
                case 1:
                    fanTranslation = glm::translate(glm::mat4(1.0f), position + glm::vec3(0.33f, 0.13f, 0.33f));
                    break;
                case 2:
                    fanTranslation = glm::translate(glm::mat4(1.0f), position + glm::vec3(-0.33f, 0.13f, 0.33f));
                    break;
                case 3:
                    fanTranslation = glm::translate(glm::mat4(1.0f), position + glm::vec3(0.33f, 0.13f, -0.33f));
                    break;
                default:
                    fanTranslation = glm::mat4(1.f);
            }

            (*uboPtr).model = fanTranslation * fanRotation * fanScaling;

            vkMapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                        sizeof(*uboPtr), 0, &dataPtr);
            memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
            vkUnmapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage]);
        }
    }

    void fall() {
        areFansActive = false;
    }

    void moveDown(float deltaT) {
        position.y -= deltaT * MOVE_SPEED;
    }

    void moveUp(float deltaT) {
        position.y += deltaT * MOVE_SPEED;
        areFansActive = true;
    }

    void moveForward(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
        areFansActive = true;
    }

    void moveBackward(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
        areFansActive = true;
    }

    void moveLeft(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
        areFansActive = true;
    }

    void moveRight(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
        areFansActive = true;
    }


};