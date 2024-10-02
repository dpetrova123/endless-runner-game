#include "AABB_CollisionDetection.hpp"

CollisionDetector::CollisionDetector() {}

CollisionDetector::CollisionDetector(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}

void CollisionDetector::getPlayer(const Player &player, float groundLevel) {
    glm::vec3 position = player.GetPosition();
    float playerWidth = 0.2f;
    float playerHeight = 0.2f;
    float playerDepth = 0.201f;

    min = position + glm::vec3(-playerWidth / 2, -playerHeight/2, -playerDepth / 2);
    max = position + glm::vec3(playerWidth / 2, playerHeight/2, playerDepth / 2);
}

void CollisionDetector::getObstacle(const glm::vec3 &position, int obsType) {
    if(obsType == 0) {
        float obstacleWidth = 0.2f;
        float obstacleHeight = 0.3f;
        float obstacleDepth = 0.202f;
        min = position + glm::vec3(-obstacleWidth / 2, 0.0f, -obstacleDepth / 2);
        max = position + glm::vec3(obstacleWidth / 2, obstacleHeight, obstacleDepth / 2);
    } else if(obsType == 1) {
        float obstacleWidth = 12.0f;
        float obstacleHeight = 0.2f;
        float obstacleDepth = 0.202f;
        min = position + glm::vec3(-obstacleWidth / 2, 0.0f, -obstacleDepth / 2);
        max = position + glm::vec3(obstacleWidth / 2, obstacleHeight, obstacleDepth / 2);
    } else if (obsType == 2){
        float obstacleWidth = 14.0f;
        float obstacleHeight = 0.5f;
        float obstacleDepth = 0.202f;
        min = position + glm::vec3(-obstacleWidth / 2, 0.3, -obstacleDepth / 2);
        max = position + glm::vec3(obstacleWidth / 2, obstacleHeight, obstacleDepth / 2);
    }
}

bool CollisionDetector::check(const CollisionDetector &b) {
    return (min.x <= b.getMax().x && max.x >= b.getMin().x) &&
           (min.y <= b.getMax().y && max.y >= b.getMin().y) &&
           (min.z <= b.getMax().z && max.z >= b.getMin().z);
}

const glm::vec3 &CollisionDetector::getMin() const {
    return min;
}

const glm::vec3 &CollisionDetector::getMax() const {
    return max;
}
