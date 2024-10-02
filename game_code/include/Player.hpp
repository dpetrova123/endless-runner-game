#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>
#include <vector>

class Player {
public:
    Player(const std::vector<float>& lanes, int startLaneIndex, float switchSpeed, float jumpSpeed, float crouchSpeed);
    void ProcessInput(bool leftKeyPressed, bool rightKeyPressed, bool upKeyPressed, bool downKeyPressed, float deltaTime);
    glm::vec3 GetPosition() const;
    int getCurrentLane();
    void MoveForward(float speed, float deltaTime);

private:
    std::vector<float> lanes;
    int currentLaneIndex;
    float laneSwitchSpeed;
    glm::vec3 position;
    float jumpSpeed;
    float crouchSpeed;
    float targetJump = 0.5f;
    float targetCrouch = -0.3f;
    bool isJumping = false;
    bool isGrounding = false;
    bool isCrouching = false;
    bool goingLeft = false;
    bool goingRight = false;
};

#endif // PLAYER_HPP
