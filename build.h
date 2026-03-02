#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace TreeConfig {
    // Параметры дерева
    const int MAX_TREE_LEVELS = 6;
    const int MIN_CHILDREN_PER_NODE = 2;
    const int MAX_CHILDREN_PER_NODE = 4;
    const int MIN_PROPERTIES_PER_NODE = 2;
    const int ROOT_START_X = 640;
    const int ROOT_START_Y = 750;
    const int TREE_CENTER_X = 640;
    const int TREE_CENTER_Y = 550;
    const float TREE_INITIAL_ANGLE = -90.0f;
    const float TREE_INITIAL_SPREAD = 220.0f;
    const float TREE_INITIAL_DISTANCE = 240.0f;

    // Параметры размещения дерева
    const float CHILD_SPREAD_MULTIPLIER = 0.92f;
    const float CHILD_DISTANCE_MULTIPLIER = 0.92f;
    const float RANDOM_DISTANCE_MIN = 0.45f;
    const float RANDOM_DISTANCE_MAX = 1.26f;

    // Визуальные параметры
    const int BASE_PARTICLE_RADIUS = 8;
    const float RADIUS_DECREASE_PER_LEVEL = 0.8f;
    const int MIN_PARTICLE_RADIUS = 3;
    const int HIT_DETECTION_RADIUS_SQ = 225;

    // Параметры роста
    const float GROW_SPEED_MIN = 0.5f;
    const float GROW_SPEED_MAX = 1.21f;
}

extern std::mt19937 gen;
extern const std::vector<std::string> EN_PROPS;

class Particle {
private:
    std::vector<std::string> m_props;
    float m_homeX = 0.0f, m_homeY = 0.0f;
    float m_x = 0.0f, m_y = 0.0f;
    float m_vx = 0.0f, m_vy = 0.0f;
    std::vector<Particle*> m_children;
    Particle* m_parent = nullptr;
    float m_growProgress = 0.0f;
    float m_growSpeed = 0.0f;
    bool m_visible = false;
    bool m_dragged = false;
    int m_level = 0;
    int m_weight = 0;
    float m_stability = 0.0f;
    bool m_markedForDelete = false;

public:
    Particle(const std::vector<std::string>& p, int lvl, float startX, float startY, Particle* par);

    ~Particle();

    Particle(const Particle&) = delete;
    Particle& operator=(const Particle&) = delete;

    Particle(Particle&&) = default;
    Particle& operator=(Particle&&) = default;

    const std::vector<std::string>& getProps() const { return m_props; }
    float getHomeX() const { return m_homeX; }
    float getHomeY() const { return m_homeY; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getVx() const { return m_vx; }
    float getVy() const { return m_vy; }
    const std::vector<Particle*>& getChildren() const { return m_children; }
    Particle* getParent() const { return m_parent; }
    float getGrowProgress() const { return m_growProgress; }
    float getGrowSpeed() const { return m_growSpeed; }
    bool isVisible() const { return m_visible; }
    bool isDragged() const { return m_dragged; }
    int getLevel() const { return m_level; }
    int getWeight() const { return m_weight; }
    float getStability() const { return m_stability; }
    bool isMarkedForDelete() const { return m_markedForDelete; }

    void setX(float x) { m_x = x; }
    void setY(float y) { m_y = y; }
    void setVx(float vx) { m_vx = vx; }
    void setVy(float vy) { m_vy = vy; }
    void setVisible(bool visible) { m_visible = visible; }
    void setDragged(bool dragged) { m_dragged = dragged; }
    void setGrowProgress(float progress) { m_growProgress = progress; }
    void setHomePosition(float x, float y) { m_homeX = x; m_homeY = y; }
    void setWeight(int weight) { m_weight = weight; }

    bool hasProp(const std::string& prop) const;
    bool isDescendantOf(const Particle* target) const;
    void markBranchForDelete();
    void addChild(Particle* child);
    void removeChild(Particle* child);
};

namespace TreeUtils {
    int countWeight(Particle* p);
    void arrangeTree(Particle* p, float x, float y, float angle, float spread, float distance);
    void shuffleArray(std::vector<std::string>& arr);
    void createTree(Particle* parent, int level);
    void collectParticles(Particle* p, std::vector<Particle*>& list);
    void removeMarkedParticles(std::vector<Particle*>& allParticles);
}

#endif // BUILD_H_INCLUDED
