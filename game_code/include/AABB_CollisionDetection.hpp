#define AABB_COLLISIONDETECTION_HPP

#include <glm/glm.hpp>
#include <vector>
#include "Player.hpp"

class CollisionDetector {
public:
    CollisionDetector();
    CollisionDetector(glm::vec3 min, glm::vec3 max);
    void getPlayer(const Player& player, float groundLevel);
    void getObstacle(const glm::vec3& position, int obsType);
    bool check(const CollisionDetector& b);
    const glm::vec3 &getMin() const;
    const glm::vec3 &getMax() const;

private:
    glm::vec3 min;
    glm::vec3 max;
};

