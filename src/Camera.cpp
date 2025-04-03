#include "Camera.h"
#include "Chunk.h"

Camera::Camera(glm::vec3 initPosition, float initYaw, float initPitch)
    : position(initPosition), yaw(initYaw), pitch(initPitch), firstMouse(true), lastX(400.0f), lastY(300.0f) {
    updateCameraVectors();
}

void Camera::processInput(GLFWwindow* window, float deltaTime, const Chunk& chunk) {
    float currentSpeed = speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        currentSpeed = sprintSpeed;
    }

    float velocity = currentSpeed * deltaTime;
    glm::vec3 newPosition = position;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        newPosition += glm::normalize(glm::vec3(front.x, 0.0f, front.z)) * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        newPosition -= glm::normalize(glm::vec3(front.x, 0.0f, front.z)) * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        newPosition -= glm::normalize(glm::vec3(right.x, 0.0f, right.z)) * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        newPosition += glm::normalize(glm::vec3(right.x, 0.0f, right.z)) * velocity;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isOnGround) {
        verticalVelocity = jumpVelocity;
        isOnGround = false;
    }

    verticalVelocity -= gravity * deltaTime;
    newPosition.y += verticalVelocity * deltaTime;

    CollisionResult result = checkCollision(newPosition, chunk);
    if (!result.collided) {
        position = newPosition;
    } else {
        if (result.normal.y != 0.0f) {  // Collision verticale
            position.y = (result.normal.y > 0) ? chunk.getTerrainHeight(static_cast<int>(position.x), static_cast<int>(position.z)) : position.y;
            verticalVelocity = 0.0f;
            isOnGround = (result.normal.y > 0);
        } else {  // Collision horizontale -> glissement
            glm::vec3 horizontalMove = newPosition - position;
            horizontalMove.y = 0.0f;
            if (result.normal.x != 0.0f) horizontalMove.x = 0.0f;
            if (result.normal.z != 0.0f) horizontalMove.z = 0.0f;
            newPosition = position + horizontalMove;
            if (!checkCollision(newPosition, chunk).collided) {
                position = newPosition;
            }
        }
    }

    int terrainHeight = chunk.getTerrainHeight(static_cast<int>(position.x), static_cast<int>(position.z));
    if (position.y <= terrainHeight) {
        position.y = terrainHeight;
        verticalVelocity = 0.0f;
        isOnGround = true;
    }

    std::cout << "Position : " << position.x << ", " << position.y << ", " << position.z 
              << " | OnGround: " << isOnGround << " | TerrainHeight: " << terrainHeight << std::endl;
}

void Camera::processMouseMovement(float xpos, float ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

void Camera::updateCameraVectors() {
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

Camera::CollisionResult Camera::checkCollision(const glm::vec3& pos, const Chunk& chunk) {
    static bool firstCheck = true;
    if (firstCheck) {
        firstCheck = false;
        std::cout << "Skipping initial collision check" << std::endl;
        return {false, glm::vec3(0.0f)};
    }

    float playerWidth = 0.3f;
    float playerHeight = 1.8f;

    int x = static_cast<int>(floor(pos.x));
    int y = static_cast<int>(floor(pos.y));
    int z = static_cast<int>(floor(pos.z));

    std::cout << "Check collision at pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = 0; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int bx = x + dx;
                int by = y + dy;
                int bz = z + dz;

                if (bx >= 0 && bx < 16 && by >= 0 && by < 16 && bz >= 0 && bz < 16) {
                    if (chunk.isBlockSolid(bx, by, bz)) {
                        std::cout << "Solid block at: (" << bx << ", " << by << ", " << bz << ")" << std::endl;

                        float blockMinX = bx - 0.5f;
                        float blockMaxX = bx + 0.5f;
                        float blockMinY = by - 0.5f;
                        float blockMaxY = by + 0.5f;
                        float blockMinZ = bz - 0.5f;
                        float blockMaxZ = bz + 0.5f;

                        float playerMinX = pos.x - playerWidth;
                        float playerMaxX = pos.x + playerWidth;
                        float playerMinY = pos.y;
                        float playerMaxY = pos.y + playerHeight;
                        float playerMinZ = pos.z - playerWidth;
                        float playerMaxZ = pos.z + playerWidth;

                        if (playerMinX < blockMaxX && playerMaxX > blockMinX &&
                            playerMinY < blockMaxY && playerMaxY > blockMinY &&
                            playerMinZ < blockMaxZ && playerMaxZ > blockMinZ) {
                            std::cout << "Collision detected with block at: (" << bx << ", " << by << ", " << bz << ")" << std::endl;
                            glm::vec3 normal(0.0f);
                            if (playerMaxX > blockMinX && pos.x < bx) normal.x = 1.0f;
                            else if (playerMinX < blockMaxX && pos.x > bx) normal.x = -1.0f;
                            if (playerMaxY > blockMinY && pos.y < by) normal.y = 1.0f;
                            else if (playerMinY < blockMaxY && pos.y > by) normal.y = -1.0f;
                            if (playerMaxZ > blockMinZ && pos.z < bz) normal.z = 1.0f;
                            else if (playerMinZ < blockMaxZ && pos.z > bz) normal.z = -1.0f;
                            return {true, normal};
                        }
                    }
                }
            }
        }
    }
    return {false, glm::vec3(0.0f)};
}
