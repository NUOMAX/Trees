#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>

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
const float CHILD_SPREAD_MULTIPLIER = 0.85f;
const float CHILD_DISTANCE_MULTIPLIER = 0.92f;
const float RANDOM_DISTANCE_MIN = 0.85f;
const float RANDOM_DISTANCE_MAX = 1.26f;

// Параметры внешнего вида
const int BASE_PARTICLE_RADIUS = 8;
const float RADIUS_DECREASE_PER_LEVEL = 0.8f;
const int MIN_PARTICLE_RADIUS = 3;
const int HIT_DETECTION_RADIUS_SQ = 225;

// Параметры роста и анимации (добавлено из animated.h)
const float GROW_SPEED_MIN = 0.5f;
const float GROW_SPEED_MAX = 1.21f;

extern std::mt19937 gen;
extern const std::vector<std::string> EN_PROPS;

struct Particle {
    std::vector<std::string> props;
    float homeX, homeY;
    float x, y;
    float vx, vy;
    std::vector<Particle*> children;
    Particle* parent;
    float growProgress;
    float growSpeed;
    bool visible;
    bool dragged;
    int level;
    int weight;
    float stability;
    bool markedForDelete;

    Particle(std::vector<std::string> p, int lvl, float startX, float startY, Particle* par);
    ~Particle();

    bool hasProp(std::string prop);
    bool isDescendantOf(Particle* target);
    void markBranchForDelete();
};

int countWeight(Particle* p);
void arrangeTree(Particle* p, float x, float y, float angle, float spread, float distance);
void shuffleArray(std::vector<std::string>& arr);
void createTree(Particle* parent, int level);
void collectParticles(Particle* p, std::vector<Particle*>& list);
void removeMarkedParticles(std::vector<Particle*>& allParticles);

#endif // BUILD_H_INCLUDED
