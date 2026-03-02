#include "build.h"

std::mt19937 gen(std::random_device{}());

// Все доступные свойства частиц
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

Particle::Particle(const std::vector<std::string>& p, int lvl, float startX, float startY, Particle* par)
    : m_props(p)
    , m_level(lvl)
    , m_x(startX)
    , m_y(startY)
    , m_parent(par)
    , m_growProgress(0)
    , m_visible(false)
    , m_dragged(false)
    , m_vx(0)
    , m_vy(0)
    , m_markedForDelete(false) {

    float speedRange = TreeConfig::GROW_SPEED_MAX - TreeConfig::GROW_SPEED_MIN;
    m_growSpeed = TreeConfig::GROW_SPEED_MIN + (rand() % 101) / 100.f * speedRange;
    m_stability = 1.0f + m_level * 0.3f;
}

Particle::~Particle() {
    for (auto child : m_children) {
        delete child;
    }
}

bool Particle::hasProp(const std::string& prop) const {
    for (const auto& p : m_props) {
        if (p == prop) return true;
    }
    return false;
}

bool Particle::isDescendantOf(const Particle* target) const {
    if (target == nullptr) return false;
    if (this == target) return true;
    if (m_parent == nullptr) return false;
    return m_parent->isDescendantOf(target);
}

void Particle::markBranchForDelete() {
    m_markedForDelete = true;
    for (auto child : m_children) {
        child->markBranchForDelete();
    }
}

void Particle::addChild(Particle* child) {
    m_children.push_back(child);
}

void Particle::removeChild(Particle* child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);
    }
}

namespace TreeUtils {
    int countWeight(Particle* p) {
        if (p->getChildren().empty()) {
            p->setWeight(1);
            return 1;
        }

        int sum = 0;
        for (const auto& child : p->getChildren()) {
            sum += countWeight(child);
        }
        p->setWeight(sum);
        return sum;
    }

    void arrangeTree(Particle* p, float x, float y, float angle, float spread, float distance) {
        using namespace TreeConfig;

        p->setHomePosition(x, y);

        if (p->getChildren().empty()) return;

        float currentAngle = angle - spread / 2;

        for (const auto& child : p->getChildren()) {
            float childSpread = (static_cast<float>(child->getWeight()) / p->getWeight()) * spread;

            float randomFactor = RANDOM_DISTANCE_MIN +
                (rand() % static_cast<int>((RANDOM_DISTANCE_MAX - RANDOM_DISTANCE_MIN) * 100)) / 100.f;
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
        for (size_t i = arr.size() - 1; i > 0; --i) {
            size_t j = rand() % (i + 1);
            std::swap(arr[i], arr[j]);
        }
    }

    void createTree(Particle* parent, int level) {
        using namespace TreeConfig;

        if (parent->getProps().size() <= MIN_PROPERTIES_PER_NODE || level > MAX_TREE_LEVELS) return;

        int childrenCount = MIN_CHILDREN_PER_NODE +
            (rand() % (MAX_CHILDREN_PER_NODE - MIN_CHILDREN_PER_NODE + 1));

        std::vector<std::string> propList = parent->getProps();

        for (int i = 0; i < childrenCount; ++i) {
            std::vector<std::string> childProps;
            int propCount = MIN_PROPERTIES_PER_NODE +
                (rand() % (static_cast<int>(propList.size()) - MIN_PROPERTIES_PER_NODE));

            shuffleArray(propList);

            for (int j = 0; j < propCount; ++j) {
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

            Particle* child = new Particle(childProps, level + 1, parent->getX(), parent->getY(), parent);
            parent->addChild(child);
            createTree(child, level + 1);
        }
    }

    void collectParticles(Particle* p, std::vector<Particle*>& list) {
        if (p->isMarkedForDelete()) return;
        list.push_back(p);
        for (const auto& child : p->getChildren()) {
            collectParticles(child, list);
        }
    }

    void removeMarkedParticles(std::vector<Particle*>& allParticles) {
        allParticles.erase(
            std::remove_if(allParticles.begin(), allParticles.end(),
                [](Particle* p) {
                    if (p->isMarkedForDelete()) {
                        delete p;
                        return true;
                    }
                    return false;
                }),
            allParticles.end()
        );
    }
}
