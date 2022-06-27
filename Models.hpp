#include "MyProject.hpp"
#include <string>
#include <utility>
#include <glm/gtc/quaternion.hpp>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
};

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
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0,                           nullptr}
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
    const float MOVE_SPEED = 20.f;

public:
    BaseModel *droneBaseModel;
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat direction = glm::quat(1, 0, 0, 0);
    float scale_factor = 0.3f;


    Drone(BaseModel *droneBaseModelPtr) {
        this->droneBaseModel = droneBaseModelPtr;
    }

    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {
        glm::mat4 translation = glm::translate(glm::mat4(1), position);
        auto rotation = glm::mat4(direction);
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
        worldMatrix = translation * rotation * scaling;
        (*uboPtr).model = worldMatrix;

        vkMapMemory(*devicePtr, (*droneBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(*uboPtr), 0, &dataPtr);
        memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
        vkUnmapMemory(*devicePtr, (*droneBaseModel).descriptorSet.uniformBuffersMemory[0][currentImage]);
    }


    void moveDown(float deltaT) {
        position.y -= deltaT * MOVE_SPEED;
    }

    void moveUp(float deltaT) {
        position.y += deltaT * MOVE_SPEED;
    }

    void moveForward(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
    }

    void moveBackward(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
    }

    void moveLeft(float deltaT, float lookYaw) {
        position -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
    }

    void moveRight(float deltaT, float lookYaw) {
        position += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                       glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
    }


};