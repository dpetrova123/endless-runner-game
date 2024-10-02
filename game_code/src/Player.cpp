#include <iostream>
#include <cmath>
#include "Player.hpp"

Player::Player(const std::vector<float>& lanes, int startLaneIndex, float switchSpeed, float jumpSpeed, float crouchSpeed)
        : lanes(lanes), currentLaneIndex(startLaneIndex), laneSwitchSpeed(switchSpeed), position(glm::vec3(lanes[startLaneIndex], 0.0f, 0.0f)), jumpSpeed(jumpSpeed), crouchSpeed(crouchSpeed) {}

void Player::ProcessInput(bool leftKeyPressed, bool rightKeyPressed, bool upKeyPressed, bool downKeyPressed, float deltaTime) {
    if (leftKeyPressed && currentLaneIndex > 0 && !goingRight)
    { goingLeft = true;}
    if (rightKeyPressed && currentLaneIndex < lanes.size() - 1 && !goingLeft)
    { goingRight = true;}
    if(goingLeft && !goingRight) {
        float targetX = lanes[currentLaneIndex - 1];
        position.x = glm::mix(position.x, targetX, laneSwitchSpeed * deltaTime);
        if (abs(position.x - targetX) <= 0.01f){
            goingLeft = false;
            currentLaneIndex--;
        }
    }
    if(goingRight && !goingLeft) {
        float targetX = lanes[currentLaneIndex + 1];
        position.x = glm::mix(position.x, targetX, laneSwitchSpeed * deltaTime);
        if (abs(position.x - targetX) <= 0.01f){
            goingRight = false;
            currentLaneIndex++;
        }
    }
    if (upKeyPressed && !isCrouching && !isGrounding)
    { isJumping = true; }

    if(isJumping) {
        position.y = glm::mix(position.y, targetJump, jumpSpeed * deltaTime);
        if (abs(position.y - targetJump) <= 0.08f){
            isJumping = false; isGrounding = true;
        }
    }
    if(isGrounding) {
        position.y = glm::mix(position.y, 0.0f, jumpSpeed * deltaTime);
        if (abs(position.y) <= 0.05f){
            isGrounding = false;
        }
    }
    if(downKeyPressed && !isJumping && !isGrounding){
        isCrouching = true;
    }
    if(isCrouching) {
        position.y = glm::mix(position.y, targetCrouch, crouchSpeed * deltaTime);
        if (abs(position.y - targetCrouch) <= 0.08f) {
            isCrouching = false; isGrounding = true;
        }
    }
}

glm::vec3 Player::GetPosition() const {
    return position;
}

void Player::MoveForward(float speed, float deltaTime) {
    position.z -= speed * deltaTime;
}

int Player::getCurrentLane() {
    return currentLaneIndex;
}
