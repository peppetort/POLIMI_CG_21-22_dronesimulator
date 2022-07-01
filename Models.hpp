#include "DroneSimulator.hpp"
#include <string>
#include <utility>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/string_cast.hpp>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
};
struct SkyBoxUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
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

    void init(std::string modelPath, std::string texturePath, bool isSkyBox = false) {
        model.init(baseProjectPtr, std::move(modelPath));
        texture.init(baseProjectPtr, std::move(texturePath));
        if (isSkyBox)
            descriptorSet.init(baseProjectPtr, descriptorSetLayoutPtr, {
                    {0, UNIFORM, sizeof(SkyBoxUniformBufferObject), nullptr},
                    {1, TEXTURE, 0,                           &texture}
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

    const float FAN_MAX_SPEED = 50.f;
    const float FAN_MIN_SPEED = 20.f;

    const float INCLINATION_SPEED = glm::radians(45.f);
    const float MAX_INCLINATION = glm::radians(10.f);

    const float ROTATION_SPEED = glm::radians(60.f);


    float fanSpeed = FAN_MIN_SPEED;
    float moveSpeed[3][2] = {{0.f, 0.f},
                             {0.f, 0.f},
                             {0.f, 0.f}};

    float getSpeedValue(glm::vec3 moveDirection) {
        if (moveDirection.z != 0) {
            if (moveDirection.z < 0) {
                return moveSpeed[0][0];
            } else {
                return moveSpeed[0][1];
            }
        }

        if (moveDirection.x != 0) {
            if (moveDirection.x < 0) {
                return moveSpeed[1][0];
            } else {
                return moveSpeed[1][1];
            }
        }

        if (moveDirection.y != 0) {
            if (moveDirection.y < 0) {
                return moveSpeed[2][0];
            } else {
                return moveSpeed[2][1];
            }
        }

        return 0.f;
    }

    void updateSpeedValue(glm::vec3 moveDirection, float speed) {
        if (moveDirection.z != 0) {
            if (moveDirection.z < 0) {
                moveSpeed[0][1] = 0.f;
                moveSpeed[0][0] = speed;
            } else {
                moveSpeed[0][0] = 0.f;
                moveSpeed[0][1] = speed;
            }
        }

        if (moveDirection.x != 0) {
            if (moveDirection.x < 0) {
                moveSpeed[1][1] = 0.f;
                moveSpeed[1][0] = speed;
            } else {
                moveSpeed[1][0] = 0.f;
                moveSpeed[1][1] = speed;
            }
        }

        if (moveDirection.y != 0) {
            if (moveDirection.y < 0) {
                moveSpeed[2][1] = 0.f;
                moveSpeed[2][0] = speed;
            } else {
                moveSpeed[2][0] = 0.f;
                moveSpeed[2][1] = speed;
            }
        }
    }

    void activeFans() {
        fanSpeed += fanSpeed < FAN_MAX_SPEED ? 0.5f : 0.f;
        if (fanSpeed > FAN_MAX_SPEED) {
            fanSpeed = FAN_MAX_SPEED;
        }
    }

    void deactivateFans() {
        fanSpeed -= fanSpeed > FAN_MIN_SPEED ? 0.2f : 0.f;
        if (fanSpeed < FAN_MIN_SPEED) {
            fanSpeed = FAN_MIN_SPEED;
        }
    }


public:
    BaseModel *droneBaseModel;
    BaseModel *fanBaseModelList;

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.f, 0.f, 0.f);
    float scale_factor = 0.3f;

    Drone(BaseModel *droneBaseModelPtr, BaseModel fanBaseModelList[4]) {
        this->droneBaseModel = droneBaseModelPtr;
        this->fanBaseModelList = fanBaseModelList;
    }


    void draw(uint32_t currentImage, UniformBufferObject *uboPtr, void *dataPtr, VkDevice *devicePtr) {
        glm::mat4 droneTranslation = glm::translate(glm::mat4(1), position);
        glm::mat4 droneRotation = glm::mat4(glm::quat(glm::vec3(0, direction.y, 0)) *
                                            glm::quat(glm::vec3(direction.x, 0, 0)) *
                                            glm::quat(glm::vec3(0, 0, direction.z)));
        glm::mat4 droneScaling = glm::scale(glm::mat4(1.0f), glm::vec3(scale_factor));
        glm::mat4 worldMatrix = droneTranslation * droneRotation * droneScaling;
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
        glm::mat4 inclinationRotation = glm::mat4(glm::quat(glm::vec3(0, direction.y, 0)) *
                                                  glm::quat(glm::vec3(direction.x, 0, 0)) *
                                                  glm::quat(glm::vec3(0, 0, direction.z)));

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

    void move(float deltaT, glm::vec3 moveDirection, glm::vec3 *cameraPosition, int keyStatus) {
        float speed = getSpeedValue(moveDirection);

        if (keyStatus == GLFW_PRESS) {
            activeFans();

            // drone inclination
            if (moveDirection.z != 0) {
                direction.x += abs(direction.x) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED * moveDirection.z : 0.f;
            }
            if (moveDirection.x != 0) {
                direction.z -= abs(direction.z) < MAX_INCLINATION ? deltaT * INCLINATION_SPEED * moveDirection.x : 0.f;
            }

            // accelerate
            speed += speed < MAX_MOVE_SPEED ? MAX_MOVE_SPEED / 100 : 0.f;
            updateSpeedValue(moveDirection, speed);
        }


        if (keyStatus == GLFW_RELEASE) {

            // drone inclination
            if (moveDirection.z != 0) {
                if (moveDirection.z < 0) {
                    direction.x += direction.x < 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
                } else {
                    direction.x -= direction.x > 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
                }
            }
            if (moveDirection.x != 0) {
                if (moveDirection.x < 0) {
                    direction.z -= direction.z > 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
                } else {
                    direction.z += direction.z < 0.f ? deltaT * INCLINATION_SPEED * 2 : 0.f;
                }
            }

            // decelerate
            if (speed == 0.f) {
                return;
            }
            deactivateFans();
            speed -= speed > 0.f ? 0.05f : 0.f;
            if (speed < 0.f) {
                speed = 0.f;
            }
            updateSpeedValue(moveDirection, speed);
        }


        // move drone
        position += speed * glm::vec3(glm::rotate(glm::mat4(1.0f), direction.y,
                                                  glm::vec3(0.0f, 1.0f, 0.0f)) *
                                      glm::vec4(moveDirection, 1)) * deltaT;

        // move camera
        *cameraPosition += speed * glm::vec3(glm::rotate(glm::mat4(1.0f), direction.y,
                                                         glm::vec3(0.0f, 1.0f, 0.0f)) *
                                             glm::vec4(moveDirection, 1)) * deltaT;
    }

    void onLookRight(float deltaT, glm::vec3 *cameraPosition) {
        direction.y -= deltaT * ROTATION_SPEED;
        glm::mat4 translation = glm::translate(glm::mat4(1.f), position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), -ROTATION_SPEED * deltaT, glm::vec3(0, 1, 0));
        (*cameraPosition) = translation * rotation * glm::inverse(translation) * glm::vec4((*cameraPosition), 1.0f);
    }

    void onLookLeft(float deltaT, glm::vec3 *cameraPosition) {
        direction.y += deltaT * ROTATION_SPEED;
        glm::mat4 translation = glm::translate(glm::mat4(1.f), position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), ROTATION_SPEED * deltaT, glm::vec3(0, 1, 0));
        (*cameraPosition) = translation * rotation * glm::inverse(translation) * glm::vec4((*cameraPosition), 1.0f);
    }


};