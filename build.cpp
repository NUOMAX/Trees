#include "build.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

std::mt19937 gen(std::random_device{}());

const std::vector<std::string> EN_PROPS = {
    "Repulsion", "Attraction", "Mass", "Speed", "Color Palette", "Outer Form",
    "Internal Structure", "Temperature", "Transparency", "Viscosity", "Hardness",
    "Center of Gravity", "Density", "Acceleration", "Inertia", "Fragility", "Pressure",
    "Solubility", "Toxicity", "Volatility", "Radioactivity", "Conductivity",
    "Heat Capacity", "State of Matter", "Brightness", "Opacity", "Photosensitivity",
    "Polarization", "Volume", "Flexibility", "Adhesion", "Sound Absorption", "Texture",
    "Crystallinity", "Integrity", "Electric Charge", "Hygroscopicity", "Half-life",
    "Spin", "Refractoriness", "Stationarity", "Dispersion", "Diffusivity", "Magnetism",
    "Inertness", "Regeneration", "Memory", "Life", "Error Rate", "Valence"
};

Particle::Particle(std::vector<std::string> p, int lvl, float startX, float startY, Particle* par) {
    props = p;
    level = lvl;
    x = startX;
    y = startY;
    parent = par;
    growProgress = 0;
    visible = false;
    dragged = false;
    vx = 0;
    vy = 0;
    markedForDelete = false;

    float speedRange = GROW_SPEED_MAX - GROW_SPEED_MIN;
    growSpeed = GROW_SPEED_MIN + (rand() % 101) / 100.f * speedRange;

    stability = 1.0f + level * 0.3f;
}

Particle::~Particle() {
    for (auto child : children) {
        delete child;
    }
}

bool Particle::hasProp(std::string prop) {
    for (const auto& p : props) {
        if (p == prop) return true;
    }
    return false;
}

bool Particle::isDescendantOf(Particle* target) {
    if (target == nullptr) return false;
    if (this == target) return true;
    if (parent == nullptr) return false;
    return parent->isDescendantOf(target);
}

void Particle::markBranchForDelete() {
    markedForDelete = true;
    for (auto child : children) {
        child->markBranchForDelete();
    }
}

int countWeight(Particle* p) {
    if (p->children.empty()) {
        p->weight = 1;
        return 1;
    }

    int sum = 0;
    for (auto child : p->children) {
        sum += countWeight(child);
    }
    p->weight = sum;
    return sum;
}

void arrangeTree(Particle* p, float x, float y, float angle, float spread, float distance) {
    p->homeX = x;
    p->homeY = y;

    if (p->children.empty()) return;

    float currentAngle = angle - spread / 2;

    for (auto child : p->children) {
        float childSpread = ((float)child->weight / p->weight) * spread;

        float randomFactor = RANDOM_DISTANCE_MIN +
            (rand() % (int)((RANDOM_DISTANCE_MAX - RANDOM_DISTANCE_MIN) * 100)) / 100.f;
        float randomDist = distance * randomFactor;

        float rad = (currentAngle + childSpread / 2) * 3.14159f / 90;

        float childX = x + cos(rad) * randomDist;
        float childY = y + sin(rad) * randomDist;

        arrangeTree(child, childX, childY,
                   currentAngle + childSpread / 2,
                   childSpread * CHILD_SPREAD_MULTIPLIER,
                   distance * CHILD_DISTANCE_MULTIPLIER);

        currentAngle += childSpread;
    }
}

void shuffleArray(std::vector<std::string>& arr) {
    for (int i = arr.size() - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(arr[i], arr[j]);
    }
}

void createTree(Particle* parent, int level) {
    if (parent->props.size() <= MIN_PROPERTIES_PER_NODE || level > MAX_TREE_LEVELS) return;

    int childrenCount = MIN_CHILDREN_PER_NODE +
        (rand() % (MAX_CHILDREN_PER_NODE - MIN_CHILDREN_PER_NODE + 1));

    std::vector<std::string> propList = parent->props;

    for (int i = 0; i < childrenCount; i++) {
        std::vector<std::string> childProps;
        int propCount = MIN_PROPERTIES_PER_NODE +
            (rand() % (propList.size() - MIN_PROPERTIES_PER_NODE));

        shuffleArray(propList);

        for (int j = 0; j < propCount; j++) {
            bool already = false;
            for (const auto& existing : childProps) {
                if (existing == propList[j]) {
                    already = true;
                    break;
                }
            }
            if (!already) {
                childProps.push_back(propList[j]);
            }
        }

        Particle* child = new Particle(childProps, level + 1, parent->x, parent->y, parent);
        parent->children.push_back(child);
        createTree(child, level + 1);
    }
}

void collectParticles(Particle* p, std::vector<Particle*>& list) {
    if (p->markedForDelete) return;
    list.push_back(p);
    for (auto child : p->children) {
        collectParticles(child, list);
    }
}

void removeMarkedParticles(std::vector<Particle*>& allParticles) {
    allParticles.erase(
        std::remove_if(allParticles.begin(), allParticles.end(),
            [](Particle* p) { return p->markedForDelete; }),
        allParticles.end()
    );
}
