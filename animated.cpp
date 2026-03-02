#include "animated.h"

extern std::mt19937 gen;

DeletingParticle::DeletingParticle(std::unique_ptr<Particle> p, sf::Color color)
    : m_particle(std::move(p))
    , m_deleteProgress(0.0f)
    , m_x(m_particle->getX())
    , m_y(m_particle->getY())
    , m_vx(m_particle->getVx())
    , m_vy(m_particle->getVy())
    , m_shrinkFactor(1.0f)
    , m_originalColor(color) {}

void DeletingParticle::update(float dt) {
    m_deleteProgress += dt * AnimationConfig::DELETE_SHRINK_SPEED;
    m_x += m_vx * dt;
    m_y += m_vy * dt;

    m_vx *= 0.95f;
    m_vy *= 0.95f;

    m_shrinkFactor = 1.0f - m_deleteProgress;
    if (m_shrinkFactor < 0) m_shrinkFactor = 0;
}

void DeletingParticle::addVelocity(float vx, float vy) {
    m_vx += vx;
    m_vy += vy;
}

std::unique_ptr<Particle> DeletingParticle::releaseParticle() {
    return std::move(m_particle);
}

namespace AnimationUtils {
    sf::Color getColor(float x, float y, int level, bool highlight, float time) {
        using namespace AnimationConfig;

        if (highlight) return HIGHLIGHT_BRANCH_COLOR;

        float yFactor = (y - COLOR_Y_MIN) / (COLOR_Y_MAX - COLOR_Y_MIN);
        yFactor = std::clamp(yFactor, 0.0f, 1.0f);

        float xFactor = std::abs(x - COLOR_CENTER_X) / COLOR_CENTER_X;
        xFactor = std::min(xFactor, 1.0f);

        float wave = sin(time * COLOR_WAVE_FREQUENCY + yFactor * 3) * COLOR_WAVE_AMPLITUDE;

        int r = static_cast<int>(255 * (1 - yFactor) * (1 - xFactor * 0.5f));
        r = std::clamp(r, 40, 255);

        int g = static_cast<int>(120 + 135 * (yFactor + wave));
        g = std::min(g, 255);

        int b = static_cast<int>(180 * xFactor + 200 * (1 - yFactor));
        b = std::clamp(b, 80, 255);

        int a = std::max(255 - level * 20, 120);

        return sf::Color(r, g, b, a);
    }

    void deleteBranch(Particle* branchRoot,
                      std::vector<DeletingParticle>& deletingParticles,
                      std::vector<Particle*>& allParticles,
                      std::vector<Spark>& sparks,
                      float totalTime) {
        using namespace AnimationConfig;

        if (branchRoot->getParent() == nullptr) {
            return;
        }

        std::vector<Particle*> branchParticles;
        TreeUtils::collectParticles(branchRoot, branchParticles);

        for (Particle* p : branchParticles) {
            if (!p->isVisible()) continue;

            sf::Color col = getColor(p->getX(), p->getY(), p->getLevel(), false, totalTime);

            auto it = std::find(allParticles.begin(), allParticles.end(), p);

            if (it != allParticles.end()) {

                allParticles.erase(it);

                std::unique_ptr<Particle> particlePtr(p);

                DeletingParticle dp(std::move(particlePtr), col);

                for (int j = 0; j < DELETE_SPARKS_COUNT; ++j) {
                    Spark s;
                    s.x = p->getX();
                    s.y = p->getY();

                    float angle = (rand() % 628) / 100.f;
                    float speed = SPARK_SPEED_MIN * 2 + (rand() % (SPARK_SPEED_MAX * 2));

                    s.vx = cos(angle) * speed;
                    s.vy = sin(angle) * speed;

                    s.r = 255;
                    s.g = 100 + (rand() % 100);
                    s.b = 50 + (rand() % 50);
                    s.life = SPARK_LIFETIME * 1.5f;

                    sparks.push_back(s);

                    dp.addVelocity(s.vx * 0.1f, s.vy * 0.1f);
                }

                deletingParticles.push_back(std::move(dp));
            }
        }

        for (Particle* p : branchParticles) {
            if (p->getParent() != nullptr) {
                p->getParent()->removeChild(p);
            }
        }
    }
}
