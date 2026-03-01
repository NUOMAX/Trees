#include "animated.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <cstdlib>

extern std::mt19937 gen;

DeletingParticle::DeletingParticle(Particle* p, sf::Color color) {
    particle = p;
    deleteProgress = 0.0f;
    x = p->x;
    y = p->y;
    vx = p->vx;
    vy = p->vy;
    shrinkFactor = 1.0f;
    originalColor = color;
}

sf::Color getColor(float x, float y, int level, bool highlight, float time) {
    if (highlight) return HIGHLIGHT_BRANCH_COLOR;

    float yFactor = (y - COLOR_Y_MIN) / (COLOR_Y_MAX - COLOR_Y_MIN);
    if (yFactor < 0) yFactor = 0;
    if (yFactor > 1) yFactor = 1;

    float xFactor = std::abs(x - COLOR_CENTER_X) / COLOR_CENTER_X;
    if (xFactor > 1) xFactor = 1;

    float wave = sin(time * COLOR_WAVE_FREQUENCY + yFactor * 3) * COLOR_WAVE_AMPLITUDE;

    int r = 255 * (1 - yFactor) * (1 - xFactor * 0.5f);
    if (r < 40) r = 40;
    if (r > 255) r = 255;

    int g = 120 + 135 * (yFactor + wave);
    if (g > 255) g = 255;

    int b = 180 * xFactor + 200 * (1 - yFactor);
    if (b < 80) b = 80;
    if (b > 255) b = 255;

    int a = 255 - level * 20;
    if (a < 120) a = 120;

    return sf::Color(r, g, b, a);
}

void deleteBranch(Particle* branchRoot, std::vector<DeletingParticle>& deletingParticles,
                  std::vector<Particle*>& allParticles, std::vector<Spark>& sparks, float totalTime) {

    if (branchRoot->parent == nullptr) {
        return;
    }

    std::vector<Particle*> branchParticles;
    collectParticles(branchRoot, branchParticles);

    for (Particle* p : branchParticles) {
        if (!p->visible) continue;

        sf::Color col = getColor(p->x, p->y, p->level, false, totalTime);

        DeletingParticle dp(p, col);
        deletingParticles.push_back(dp);

        for (int j = 0; j < DELETE_SPARKS_COUNT; j++) {
            Spark s;
            s.x = p->x;
            s.y = p->y;

            float angle = (rand() % 628) / 100.f;
            float speed = SPARK_SPEED_MIN * 2 + (rand() % (SPARK_SPEED_MAX * 2));

            s.vx = cos(angle) * speed;
            s.vy = sin(angle) * speed;

            s.r = 255;
            s.g = 100 + (rand() % 100);
            s.b = 50 + (rand() % 50);
            s.life = SPARK_LIFETIME * 1.5f;

            sparks.push_back(s);

            deletingParticles.back().vx += s.vx * 0.1f;
            deletingParticles.back().vy += s.vy * 0.1f;
        }

        p->markBranchForDelete();
    }

    removeMarkedParticles(allParticles);
}
