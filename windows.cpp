#include "windows.h"
#include <sstream>

WindowManager::WindowManager()
    : m_window(sf::VideoMode({WindowConfig::WIDTH, WindowConfig::HEIGHT}), "Evolution Tree")
    , m_view(sf::FloatRect({0, 0}, {WindowConfig::WIDTH, WindowConfig::HEIGHT})) {

    m_window.setFramerateLimit(WindowConfig::FRAME_RATE_LIMIT);

    std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/times.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/Windows/Fonts/seguiemj.ttf",
        "arial.ttf"
    };

    for (const auto& path : fontPaths) {
        if (m_font.openFromFile(path)) {
            m_fontLoaded = true;
            break;
        }
    }
}

void WindowManager::pollEvents(std::vector<Particle*>& allParticles,
                               Particle*& draggedParticle, Particle*& lastClicked,
                               int& highlightLevel, Particle*& highlightBranch,
                               sf::Clock& clickTimer, bool& movingCamera,
                               int& lastMouseX, int& lastMouseY,
                               std::vector<DeletingParticle>& deletingParticles,
                               std::vector<Spark>& sparks, float totalTime) {

    while (auto optionalEvent = m_window.pollEvent()) {
        sf::Event event = *optionalEvent;

        if (event.is<sf::Event::Closed>()) {
            m_window.close();
        }

        sf::Vector2f mouse = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window), m_view);

        if (auto mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                Particle* hit = nullptr;
                for (Particle* p : allParticles) {
                    if (!p->isVisible() || p->isMarkedForDelete()) continue;

                    float dx = p->getX() - mouse.x;
                    float dy = p->getY() - mouse.y;
                    if (dx*dx + dy*dy < TreeConfig::HIT_DETECTION_RADIUS_SQ) {
                        hit = p;
                        break;
                    }
                }

                if (hit != nullptr) {
                    draggedParticle = hit;
                    hit->setDragged(true);

                    if (lastClicked == hit &&
                        clickTimer.getElapsedTime().asMilliseconds() < WindowConfig::DOUBLE_CLICK_TIME_MS) {
                        highlightBranch = hit;
                        highlightLevel = -1;
                    } else {
                        highlightLevel = hit->getLevel();
                        highlightBranch = nullptr;
                    }

                    lastClicked = hit;
                    clickTimer.restart();
                } else {
                    highlightLevel = -1;
                    highlightBranch = nullptr;
                    lastClicked = nullptr;
                    movingCamera = true;
                    lastMouseX = sf::Mouse::getPosition(m_window).x;
                    lastMouseY = sf::Mouse::getPosition(m_window).y;
                }
            }

            if (mousePressed->button == sf::Mouse::Button::Right) {
                for (Particle* p : allParticles) {
                    if (!p->isVisible() || p->isMarkedForDelete()) continue;

                    float dx = p->getX() - mouse.x;
                    float dy = p->getY() - mouse.y;
                    if (dx*dx + dy*dy < TreeConfig::HIT_DETECTION_RADIUS_SQ) {
                        AnimationUtils::deleteBranch(p, deletingParticles,
                                                    allParticles, sparks, totalTime);
                        break;
                    }
                }
            }
        }

        if (auto mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (draggedParticle != nullptr) {
                draggedParticle->setDragged(false);
                draggedParticle = nullptr;
            }
            movingCamera = false;
        }

        if (auto scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (scroll->delta > 0) {
                m_view.zoom(WindowConfig::CAMERA_ZOOM_IN_FACTOR);
            } else {
                m_view.zoom(WindowConfig::CAMERA_ZOOM_OUT_FACTOR);
            }
        }
    }
}

void WindowManager::applyZoom(float delta) {
    if (delta > 0) {
        m_view.zoom(WindowConfig::CAMERA_ZOOM_IN_FACTOR);
    } else {
        m_view.zoom(WindowConfig::CAMERA_ZOOM_OUT_FACTOR);
    }
}

void WindowManager::moveCamera(const sf::Vector2f& delta) {
    m_view.move(delta);
}

void PhysicsEngine::update(std::vector<Particle*>& allParticles,
                           float dt, std::vector<Spark>& sparks,
                           float totalTime, WindowManager& windowManager) {

    using namespace AnimationConfig;

    for (Particle* p : allParticles) {
        if (p->isMarkedForDelete()) continue;

        Particle* parent = p->getParent();
        if (parent != nullptr && parent->isVisible() && !p->isVisible() && !parent->isMarkedForDelete()) {
            float newProgress = p->getGrowProgress() + p->getGrowSpeed() * dt;
            p->setGrowProgress(std::min(newProgress, 1.0f));

            float newX = parent->getX() + (p->getHomeX() - parent->getX()) * p->getGrowProgress();
            float newY = parent->getY() + (p->getHomeY() - parent->getY()) * p->getGrowProgress();
            p->setX(newX);
            p->setY(newY);

            if (p->getGrowProgress() >= 1.0f) {
                p->setVisible(true);
                sf::Color col = AnimationUtils::getColor(p->getX(), p->getY(),
                                                        p->getLevel(), false, totalTime);

                for (int j = 0; j < SPARKS_PER_BIRTH; ++j) {
                    Spark s;
                    s.x = p->getX();
                    s.y = p->getY();

                    float angle = (rand() % 628) / 100.f;
                    float speed = SPARK_SPEED_MIN + (rand() % (SPARK_SPEED_MAX - SPARK_SPEED_MIN + 1));

                    s.vx = cos(angle) * speed;
                    s.vy = sin(angle) * speed;

                    s.r = col.r;
                    s.g = col.g;
                    s.b = col.b;
                    s.life = SPARK_LIFETIME;

                    sparks.push_back(s);
                }
            }
        }

        if (!p->isVisible()) continue;

        if (p->isDragged()) {
            sf::Vector2f mouse = windowManager.mapPixelToCoords(windowManager.getMousePosition());
            p->setX(mouse.x);
            p->setY(mouse.y);
            p->setVx(0);
            p->setVy(0);
        } else {

            if (p->getLevel() <= MAX_LEVEL_FOR_FULL_PHYSICS) {
                p->setVx(p->getVx() + (p->getHomeX() - p->getX()) * HOME_RETURN_FORCE * dt);
                p->setVy(p->getVy() + (p->getHomeY() - p->getY()) * HOME_RETURN_FORCE * dt);
            } else {
                float deepLevelFactor = pow(DEEP_LEVEL_RETURN_REDUCTION,
                                           p->getLevel() - MAX_LEVEL_FOR_FULL_PHYSICS);
                p->setVx(p->getVx() + (p->getHomeX() - p->getX()) * HOME_RETURN_FORCE * dt * deepLevelFactor);
                p->setVy(p->getVy() + (p->getHomeY() - p->getY()) * HOME_RETURN_FORCE * dt * deepLevelFactor);
            }

            for (Particle* other : allParticles) {
                if (p == other || !other->isVisible() || other->isMarkedForDelete()) continue;

                float dx = p->getX() - other->getX();
                float dy = p->getY() - other->getY();
                float dist2 = dx*dx + dy*dy;

                if (dist2 < REPULSION_RADIUS && dist2 > 0.1f) {
                    float repulsionMultiplier = 1.0f;
                    if (p->getLevel() > MAX_LEVEL_FOR_FULL_PHYSICS) {
                        repulsionMultiplier = pow(DEEP_LEVEL_RETURN_REDUCTION,
                                                 p->getLevel() - MAX_LEVEL_FOR_FULL_PHYSICS);
                    }

                    float force = REPULSION_FORCE * dt * repulsionMultiplier / dist2;
                    p->setVx(p->getVx() + dx * force);
                    p->setVy(p->getVy() + dy * force);
                }
            }

            p->setX(p->getX() + p->getVx());
            p->setY(p->getY() + p->getVy());

            float damping = VELOCITY_DAMPING;
            if (p->getLevel() > MAX_LEVEL_FOR_FULL_PHYSICS) {
                float boost = pow(DEEP_LEVEL_DAMPING_BOOST,
                                  p->getLevel() - MAX_LEVEL_FOR_FULL_PHYSICS);
                damping = VELOCITY_DAMPING * boost;
                if (damping > 0.98f) damping = 0.98f;
            }

            p->setVx(p->getVx() * damping);
            p->setVy(p->getVy() * damping);

            if (p->getLevel() >= DEEP_LEVEL_START) {
                p->setX(p->getX() * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                        p->getHomeX() * DEEP_LEVEL_ANCHOR_STRENGTH);
                p->setY(p->getY() * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                        p->getHomeY() * DEEP_LEVEL_ANCHOR_STRENGTH);
                p->setVx(p->getVx() * 0.5f);
                p->setVy(p->getVy() * 0.5f);
            }
        }
    }
}

void Renderer::draw(sf::RenderWindow& window,
                    const std::vector<Particle*>& allParticles,
                    const std::vector<DeletingParticle>& deletingParticles,
                    const std::vector<Spark>& sparks,
                    int highlightLevel, Particle* highlightBranch,
                    float totalTime, const sf::Font& font, bool fontLoaded) {

    for (const Particle* p : allParticles) {
        if (!p->isVisible() || p->isMarkedForDelete()) continue;

        for (const Particle* child : p->getChildren()) {
            if (child->getGrowProgress() <= 0.01f || child->isMarkedForDelete()) continue;

            bool highlight = (highlightBranch != nullptr && child->isDescendantOf(highlightBranch));
            sf::Color colorParent = AnimationUtils::getColor(p->getX(), p->getY(),
                                                            p->getLevel(), highlight, totalTime);

            float endX, endY;
            if (child->isVisible()) {
                endX = child->getX();
                endY = child->getY();
            } else {
                endX = p->getX() + (child->getHomeX() - p->getX()) * child->getGrowProgress();
                endY = p->getY() + (child->getHomeY() - p->getY()) * child->getGrowProgress();
            }

            sf::Color colorChild = AnimationUtils::getColor(endX, endY, child->getLevel(),
                                                           highlight, totalTime);

            sf::Vertex line[2];
            line[0].position = sf::Vector2f(p->getX(), p->getY());
            line[0].color = colorParent;
            line[1].position = sf::Vector2f(endX, endY);
            line[1].color = colorChild;

            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }

    for (const Particle* p : allParticles) {
        if (!p->isVisible() || p->isMarkedForDelete()) continue;

        float radius = TreeConfig::BASE_PARTICLE_RADIUS -
                      p->getLevel() * TreeConfig::RADIUS_DECREASE_PER_LEVEL;
        if (radius < TreeConfig::MIN_PARTICLE_RADIUS)
            radius = TreeConfig::MIN_PARTICLE_RADIUS;

        sf::CircleShape dot(radius);
        dot.setOrigin(sf::Vector2f(radius, radius));
        dot.setPosition(sf::Vector2f(p->getX(), p->getY()));

        bool highlight = (highlightBranch != nullptr && p->isDescendantOf(highlightBranch));
        sf::Color color;

        if (p->getLevel() == highlightLevel && !highlight) {
            color = AnimationConfig::HIGHLIGHT_LEVEL_COLOR;
        } else {
            color = AnimationUtils::getColor(p->getX(), p->getY(), p->getLevel(),
                                            highlight, totalTime);
        }

        dot.setFillColor(color);
        window.draw(dot);
    }

    for (const auto& dp : deletingParticles) {
        float radius = (TreeConfig::BASE_PARTICLE_RADIUS -
                       dp.getParticle()->getLevel() * TreeConfig::RADIUS_DECREASE_PER_LEVEL) *
                       dp.getShrinkFactor();
        if (radius < 1) radius = 1;

        sf::CircleShape dot(radius);
        dot.setOrigin(sf::Vector2f(radius, radius));
        dot.setPosition(sf::Vector2f(dp.getX(), dp.getY()));

        float flash = sin(dp.getDeleteProgress() * 3.14159f * 10) * 0.5f + 0.5f;
        sf::Color color(
            static_cast<int>(dp.getOriginalColor().r * (1-flash) + 255 * flash),
            static_cast<int>(dp.getOriginalColor().g * (1-flash) + 50 * flash),
            static_cast<int>(dp.getOriginalColor().b * (1-flash) + 50 * flash),
            static_cast<int>(dp.getOriginalColor().a * (1.0f - dp.getDeleteProgress() * 0.5f))
        );

        dot.setFillColor(color);
        window.draw(dot);
    }

    for (const auto& s : sparks) {
        sf::CircleShape spark(AnimationConfig::SPARK_SIZE);
        spark.setPosition(sf::Vector2f(s.x, s.y));

        int alpha = static_cast<int>(s.life * 255);
        alpha = std::clamp(alpha, 0, 255);

        spark.setFillColor(sf::Color(s.r, s.g, s.b, alpha));
        window.draw(spark);
    }

    sf::View currentView = window.getView();
    window.setView(window.getDefaultView());

    if (fontLoaded) {
        sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window), currentView);

        for (const Particle* p : allParticles) {
            if (!p->isVisible() || p->isMarkedForDelete()) continue;

            float dx = p->getX() - mouseWorld.x;
            float dy = p->getY() - mouseWorld.y;

            if (dx*dx + dy*dy < TreeConfig::HIT_DETECTION_RADIUS_SQ) {
                std::stringstream ss;
                ss << "Level: " << p->getLevel() << "\n";
                ss << "Properties: " << p->getProps().size() << "\n\n";
                ss << "LMB: drag / highlight\n";
                ss << "RMB: delete branch\n";
                ss << "Double click: highlight branch\n\n";


                int count = 0;
                for (const auto& prop : p->getProps()) {
                    ss << prop;
                    count++;
                    if (count % 2 == 0) {
                        ss << "\n";
                    } else {
                        ss << " | ";
                    }
                }

                sf::Text infoText(font, ss.str(), WindowConfig::TOOLTIP_FONT_SIZE);
                infoText.setFillColor(sf::Color::White);

                sf::FloatRect bounds = infoText.getGlobalBounds();

                sf::RectangleShape background(sf::Vector2f(
                    bounds.size.x + WindowConfig::TOOLTIP_BACKGROUND_PADDING,
                    bounds.size.y + WindowConfig::TOOLTIP_BACKGROUND_PADDING));

                sf::Vector2f pos = sf::Vector2f(sf::Mouse::getPosition(window)) +
                                   sf::Vector2f(WindowConfig::TOOLTIP_OFFSET_X,
                                               WindowConfig::TOOLTIP_OFFSET_Y);

                if (pos.x + background.getSize().x > WindowConfig::WIDTH) {
                    pos.x -= background.getSize().x + WindowConfig::TOOLTIP_OFFSET_X * 2;
                }
                if (pos.y + background.getSize().y > WindowConfig::HEIGHT) {
                    pos.y -= background.getSize().y + WindowConfig::TOOLTIP_OFFSET_Y * 2;
                }

                background.setPosition(pos);
                background.setFillColor(WindowConfig::TOOLTIP_BACKGROUND_COLOR);
                infoText.setPosition(pos + sf::Vector2f(WindowConfig::TOOLTIP_PADDING,
                                                        WindowConfig::TOOLTIP_PADDING));

                window.draw(background);
                window.draw(infoText);
                break;
            }
        }
    }

    window.setView(currentView);
}
