#ifndef ANIMATED_H_INCLUDED
#define ANIMATED_H_INCLUDED

#include "build.h"

namespace AnimationConfig {
    // Параметры физики
    const float HOME_RETURN_FORCE = 4.0f;
    const float VELOCITY_DAMPING = 0.82f;

    // Параметры отталкивания
    const float REPULSION_RADIUS = 6500.0f;
    const float REPULSION_FORCE = 1600.0f;

    // Параметры цвета
    const int COLOR_Y_MIN = 50;
    const int COLOR_Y_MAX = 850;
    const int COLOR_CENTER_X = 640;
    const float COLOR_WAVE_FREQUENCY = 0.7f;
    const float COLOR_WAVE_AMPLITUDE = 0.15f;

    // Цвета подсветки
    const sf::Color HIGHLIGHT_BRANCH_COLOR = sf::Color::Blue;
    const sf::Color HIGHLIGHT_LEVEL_COLOR = sf::Color::Yellow;

    // Параметры искр
    const int SPARKS_PER_BIRTH = 15;
    const float SPARK_LIFETIME = 1.2f;
    const float SPARK_SIZE = 1.2f;
    const int SPARK_SPEED_MIN = 60;
    const int SPARK_SPEED_MAX = 150;

    // Параметры удаления
    const float DELETE_ANIMATION_DURATION = 1.0f;
    const int DELETE_SPARKS_COUNT = 30;
    const float DELETE_SHRINK_SPEED = 2.0f;

    // Физика глубоких уровней
    const float MAX_LEVEL_FOR_FULL_PHYSICS = 3;
    const float DEEP_LEVEL_DAMPING_BOOST = 1.3f;
    const float DEEP_LEVEL_RETURN_REDUCTION = 0.6f;
    const float DEEP_LEVEL_ANCHOR_STRENGTH = 0.7f;
    const int DEEP_LEVEL_START = 4;
}

struct Spark {
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    int r = 0, g = 0, b = 0;
    float life = 0.0f;
};

class DeletingParticle {
private:
    std::unique_ptr<Particle> m_particle;
    float m_deleteProgress = 0.0f;
    float m_x = 0.0f, m_y = 0.0f;
    float m_vx = 0.0f, m_vy = 0.0f;
    float m_shrinkFactor = 1.0f;
    sf::Color m_originalColor;

public:
    DeletingParticle(std::unique_ptr<Particle> p, sf::Color color);

    // Геттеры
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getShrinkFactor() const { return m_shrinkFactor; }
    float getDeleteProgress() const { return m_deleteProgress; }
    const sf::Color& getOriginalColor() const { return m_originalColor; }
    Particle* getParticle() const { return m_particle.get(); }

    void update(float dt);
    void addVelocity(float vx, float vy);
    bool isFinished() const { return m_deleteProgress >= 1.0f; }

    std::unique_ptr<Particle> releaseParticle();
};

namespace AnimationUtils {
    sf::Color getColor(float x, float y, int level, bool highlight, float time);
    void deleteBranch(Particle* branchRoot,
                      std::vector<DeletingParticle>& deletingParticles,
                      std::vector<Particle*>& allParticles,
                      std::vector<Spark>& sparks,
                      float totalTime);
}

#endif // ANIMATED_H_INCLUDED
