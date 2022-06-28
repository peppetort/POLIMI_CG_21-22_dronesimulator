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

    void cleanUpNoTexture() {
        model.cleanup();
        descriptorSet.cleanup();
    }

};

class Drone {
private:
    const float MAX_MOVE_SPEED = 10.f;
    const float FAN_MAX_SPEED = 100.f;
    const float FAN_MIN_SPEED = 20.f;
    const float INCLINATION_SPEED = 1.f;
    const float MAX_INCLINATION = glm::radians(10.f);

    float fanSpeed = FAN_MIN_SPEED;
    float moveSpeedHorizontal1[2] = {0.f, 0.f};
    float moveSpeedHorizontal2[2] = {0.f, 0.f};
    float moveSpeedVertical[2] = {0.f, 0.f};

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
        glm::mat4 fansActiveRotation = fanSpeed > 0.f ? glm::rotate(glm::mat4(1.0f),
                                                                    time * fanSpeed * glm::radians(90.0f),
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) : glm::mat4(1.f);
        glm::mat4 inclinationRotation = glm::mat4(direction);

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

            (*uboPtr).model =
                    fanTranslationWithDrone * inclinationRotation * fanTranslationWRTDroneCenter * fanScaling *
                    fansActiveRotation;

            vkMapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage], 0,
                        sizeof(*uboPtr), 0, &dataPtr);
            memcpy(dataPtr, uboPtr, sizeof(*uboPtr));
            vkUnmapMemory(*devicePtr, fanBaseModelList[i].descriptorSet.uniformBuffersMemory[0][currentImage]);
        }
    }

    void onMoveUp(float deltaT, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedVertical[1] = 0.f;
        moveSpeedVertical[0] += moveSpeedVertical[0] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 150 : 0.f;
        position.y += deltaT * moveSpeedVertical[0];
        (*cameraPosition).y += deltaT * moveSpeedVertical[0];
    }


    void onMoveUpRelease(float deltaT, glm::vec3 *cameraPosition) {
        moveSpeedVertical[0] -= moveSpeedVertical[0] > 0.f ? 0.1f : 0.f;
        if (moveSpeedVertical[0] < 0.f) {
            moveSpeedVertical[0] = 0.f;
        }
        position.y += deltaT * moveSpeedVertical[0];
        (*cameraPosition).y += deltaT * moveSpeedVertical[0];
    }

    void onMoveDown(float deltaT, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedVertical[0] = 0.f;
        moveSpeedVertical[1] += moveSpeedVertical[1] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 50 : 0.f;
        position.y -= deltaT * moveSpeedVertical[1];
        (*cameraPosition).y -= deltaT * moveSpeedVertical[1];
    }

    void onMoveDownRelease(float deltaT, glm::vec3 *cameraPosition) {
        moveSpeedVertical[1] -= moveSpeedVertical[1] > 0.f ? 0.1f : 0.f;
        if (moveSpeedVertical[1] < 0.f) {
            moveSpeedVertical[1] = 0.f;
        }
        position.y -= deltaT * moveSpeedVertical[1];
        (*cameraPosition).y -= deltaT * moveSpeedVertical[1];
    }

    void onMoveForward(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedHorizontal1[1] = 0.f;
        moveSpeedHorizontal1[0] += moveSpeedHorizontal1[0] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 100 : 0.f;
        position -= moveSpeedHorizontal1[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(0, 0, 1, 1)) *
                    deltaT;
        *cameraPosition -= moveSpeedHorizontal1[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(0, 0, 1, 1)) *
                           deltaT;
        direction.x -= abs(direction.x) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
    }

    void onMoveForwardRelease(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        moveSpeedHorizontal1[0] -= moveSpeedHorizontal1[0] > 0.f ? 0.05f : 0.f;
        if (moveSpeedHorizontal1[0] < 0.f) {
            moveSpeedHorizontal1[0] = 0.f;
        }
        position -= moveSpeedHorizontal1[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(0, 0, 1, 1)) *
                    deltaT;
        *cameraPosition -= moveSpeedHorizontal1[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(0, 0, 1, 1)) *
                           deltaT;
        direction.x += direction.x < 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
    }

    void onMoveBackward(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedHorizontal1[0] = 0.f;
        moveSpeedHorizontal1[1] += moveSpeedHorizontal1[1] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 100 : 0.f;
        position += moveSpeedHorizontal1[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(0, 0, 1, 1)) *
                    deltaT;
        *cameraPosition += moveSpeedHorizontal1[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(0, 0, 1, 1)) *
                           deltaT;
        direction.x += abs(direction.x) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
    }

    void onMoveBackwardRelease(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        moveSpeedHorizontal1[1] -= moveSpeedHorizontal1[1] > 0.f ? 0.05f : 0.f;
        if (moveSpeedHorizontal1[1] < 0.f) {
            moveSpeedHorizontal1[1] = 0.f;
        }
        position += moveSpeedHorizontal1[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(0, 0, 1, 1)) *
                    deltaT;
        *cameraPosition += moveSpeedHorizontal1[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(0, 0, 1, 1)) *
                           deltaT;
        direction.x -= direction.x > 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
    }

    void onMoveLeft(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedHorizontal2[1] = 0.f;
        moveSpeedHorizontal2[0] += moveSpeedHorizontal2[0] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 100 : 0.f;
        position -= moveSpeedHorizontal2[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(1, 0, 0, 1)) *
                    deltaT;
        *cameraPosition -= moveSpeedHorizontal2[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(1, 0, 0, 1)) *
                           deltaT;
        direction.z += abs(direction.z) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
    }

    void onMoveLeftRelease(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        moveSpeedHorizontal2[0] -= moveSpeedHorizontal2[0] > 0.f ? 0.05f : 0.f;
        if (moveSpeedHorizontal2[0] < 0.f) {
            moveSpeedHorizontal2[0] = 0.f;
        }
        position -= moveSpeedHorizontal2[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(1, 0, 0, 1)) *
                    deltaT;
        *cameraPosition -= moveSpeedHorizontal2[0] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(1, 0, 0, 1)) *
                           deltaT;
        direction.z -= direction.z > 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
    }

    void onMoveRight(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        activeFans();
        moveSpeedHorizontal2[0] = 0.f;
        moveSpeedHorizontal2[1] += moveSpeedHorizontal2[1] < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 100 : 0.f;
        position += moveSpeedHorizontal2[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(1, 0, 0, 1)) *
                    deltaT;
        *cameraPosition += moveSpeedHorizontal2[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(1, 0, 0, 1)) *
                           deltaT;
        direction.z -= abs(direction.z) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED : 0.f;
    }

    void onMoveRightRelease(float deltaT, float lookYaw, glm::vec3 *cameraPosition) {
        moveSpeedHorizontal2[1] -= moveSpeedHorizontal2[1] > 0.f ? 0.05f : 0.f;
        if (moveSpeedHorizontal2[1] < 0.f) {
            moveSpeedHorizontal2[1] = 0.f;
        }
        position += moveSpeedHorizontal2[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                    glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                        glm::vec4(1, 0, 0, 1)) *
                    deltaT;
        *cameraPosition += moveSpeedHorizontal2[1] * glm::vec3(glm::rotate(glm::mat4(1.0f), lookYaw,
                                                                           glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                               glm::vec4(1, 0, 0, 1)) *
                           deltaT;
        direction.z += direction.z < 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
    }

    void onViewRight(float deltaT) {
        //direction.y -= deltaT * ROTATION_SPEED;
        activeFans();
    }

    void activeFans() {
        fanSpeed += fanSpeed < FAN_MAX_SPEED ? 0.2f : 0.f;
        if (fanSpeed > FAN_MAX_SPEED) {
            fanSpeed = FAN_MAX_SPEED;
        }
    }

    void deactivateFans() {
        fanSpeed = FAN_MIN_SPEED;
    }


};