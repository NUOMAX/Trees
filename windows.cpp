#include "windows.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <vector>

sf::Font loadFont() {
    sf::Font font;
    std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/times.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/Windows/Fonts/seguiemj.ttf",
        "arial.ttf"
    };

    for (const auto& path : fontPaths) {
        if (font.openFromFile(path)) {
            return font;
        }
    }
    return font;
}

void handleEvents(sf::RenderWindow& window, sf::View& view,
                  std::vector<Particle*>& allParticles,
                  Particle*& draggedParticle, Particle*& lastClicked,
                  int& highlightLevel, Particle*& highlightBranch,
                  sf::Clock& clickTimer, bool& movingCamera,
                  int& lastMouseX, int& lastMouseY,
                  std::vector<DeletingParticle>& deletingParticles,
                  std::vector<Spark>& sparks, float totalTime) {

    while (auto optionalEvent = window.pollEvent()) {
        sf::Event event = *optionalEvent;

        if (event.is<sf::Event::Closed>()) {
            window.close();
        }

        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);

        if (auto mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button == sf::Mouse::Button::Left) {
                Particle* hit = nullptr;
                for (Particle* p : allParticles) {
                    if (!p->visible || p->markedForDelete) continue;

                    float dx = p->x - mouse.x;
                    float dy = p->y - mouse.y;
                    if (dx*dx + dy*dy < HIT_DETECTION_RADIUS_SQ) {
                        hit = p;
                        break;
                    }
                }

                if (hit != nullptr) {
                    draggedParticle = hit;
                    hit->dragged = true;

                    if (lastClicked == hit &&
                        clickTimer.getElapsedTime().asMilliseconds() < DOUBLE_CLICK_TIME_MS) {
                        highlightBranch = hit;
                        highlightLevel = -1;
                    } else {
                        highlightLevel = hit->level;
                        highlightBranch = nullptr;
                    }

                    lastClicked = hit;
                    clickTimer.restart();
                } else {
                    highlightLevel = -1;
                    highlightBranch = nullptr;
                    lastClicked = nullptr;
                    movingCamera = true;
                    lastMouseX = sf::Mouse::getPosition(window).x;
                    lastMouseY = sf::Mouse::getPosition(window).y;
                }
            }

            if (mousePressed->button == sf::Mouse::Button::Right) {
                for (Particle* p : allParticles) {
                    if (!p->visible || p->markedForDelete) continue;

                    float dx = p->x - mouse.x;
                    float dy = p->y - mouse.y;
                    if (dx*dx + dy*dy < HIT_DETECTION_RADIUS_SQ) {
                        deleteBranch(p, deletingParticles, allParticles, sparks, totalTime);
                        break;
                    }
                }
            }
        }

        if (auto mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (draggedParticle != nullptr) {
                draggedParticle->dragged = false;
                draggedParticle = nullptr;
            }
            movingCamera = false;
        }

        if (auto scroll = event.getIf<sf::Event::MouseWheelScrolled>()) {
            if (scroll->delta > 0) {
                view.zoom(CAMERA_ZOOM_IN_FACTOR);
            } else {
                view.zoom(CAMERA_ZOOM_OUT_FACTOR);
            }
        }
    }
}

void updatePhysics(std::vector<Particle*>& allParticles, float dt,
                   std::vector<Spark>& sparks, float totalTime,
                   sf::RenderWindow& window, sf::View& view) {

    for (Particle* p : allParticles) {
        if (p->markedForDelete) continue;

        if (p->parent != nullptr && p->parent->visible && !p->visible && !p->parent->markedForDelete) {
            p->growProgress += p->growSpeed * dt;
            if (p->growProgress > 1) p->growProgress = 1;

            p->x = p->parent->x + (p->homeX - p->parent->x) * p->growProgress;
            p->y = p->parent->y + (p->homeY - p->parent->y) * p->growProgress;

            if (p->growProgress >= 1) {
                p->visible = true;
                sf::Color col = getColor(p->x, p->y, p->level, false, totalTime);

                for (int j = 0; j < SPARKS_PER_BIRTH; j++) {
                    Spark s;
                    s.x = p->x;
                    s.y = p->y;

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

        if (!p->visible) continue;

        if (p->dragged) {
            sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);
            p->x = mouse.x;
            p->y = mouse.y;
            p->vx = 0;
            p->vy = 0;
        } else {
            if (p->level <= MAX_LEVEL_FOR_FULL_PHYSICS) {
                p->vx += (p->homeX - p->x) * HOME_RETURN_FORCE * dt;
                p->vy += (p->homeY - p->y) * HOME_RETURN_FORCE * dt;
            } else {
                float deepLevelFactor = pow(DEEP_LEVEL_RETURN_REDUCTION,
                                           p->level - MAX_LEVEL_FOR_FULL_PHYSICS);
                p->vx += (p->homeX - p->x) * HOME_RETURN_FORCE * dt * deepLevelFactor;
                p->vy += (p->homeY - p->y) * HOME_RETURN_FORCE * dt * deepLevelFactor;
            }

            for (Particle* other : allParticles) {
                if (p == other || !other->visible || other->markedForDelete) continue;

                float dx = p->x - other->x;
                float dy = p->y - other->y;
                float dist2 = dx*dx + dy*dy;

                if (dist2 < REPULSION_RADIUS && dist2 > 0.1f) {
                    float repulsionMultiplier = 1.0f;
                    if (p->level > MAX_LEVEL_FOR_FULL_PHYSICS) {
                        repulsionMultiplier = pow(DEEP_LEVEL_RETURN_REDUCTION,
                                                 p->level - MAX_LEVEL_FOR_FULL_PHYSICS);
                    }

                    float force = REPULSION_FORCE * dt * repulsionMultiplier / dist2;
                    p->vx += dx * force;
                    p->vy += dy * force;
                }
            }

            p->x += p->vx;
            p->y += p->vy;

            float damping = VELOCITY_DAMPING;
            if (p->level > MAX_LEVEL_FOR_FULL_PHYSICS) {
                float boost = pow(DEEP_LEVEL_DAMPING_BOOST,
                                  p->level - MAX_LEVEL_FOR_FULL_PHYSICS);
                damping = VELOCITY_DAMPING * boost;
                if (damping > 0.98f) damping = 0.98f;
            }

            p->vx *= damping;
            p->vy *= damping;

            if (p->level >= DEEP_LEVEL_START) {
                p->x = p->x * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                       p->homeX * DEEP_LEVEL_ANCHOR_STRENGTH;
                p->y = p->y * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                       p->homeY * DEEP_LEVEL_ANCHOR_STRENGTH;
                p->vx *= 0.5f;
                p->vy *= 0.5f;
            }
        }
    }
}

void updateDeletingParticles(std::vector<DeletingParticle>& deletingParticles, float dt) {
    for (int i = deletingParticles.size() - 1; i >= 0; i--) {
        DeletingParticle& dp = deletingParticles[i];

        dp.deleteProgress += dt * DELETE_SHRINK_SPEED;
        dp.x += dp.vx * dt;
        dp.y += dp.vy * dt;

        dp.vx *= 0.95f;
        dp.vy *= 0.95f;

        dp.shrinkFactor = 1.0f - dp.deleteProgress;
        if (dp.shrinkFactor < 0) dp.shrinkFactor = 0;

        if (dp.deleteProgress >= 1.0f) {
            if (dp.particle->parent != nullptr) {
                auto& siblings = dp.particle->parent->children;
                siblings.erase(std::remove(siblings.begin(), siblings.end(), dp.particle), siblings.end());
                delete dp.particle;
            }
            deletingParticles.erase(deletingParticles.begin() + i);
        }
    }
}

void updateSparks(std::vector<Spark>& sparks, float dt) {
    for (int i = sparks.size() - 1; i >= 0; i--) {
        sparks[i].x += sparks[i].vx * dt;
        sparks[i].y += sparks[i].vy * dt;
        sparks[i].life -= dt;

        if (sparks[i].life <= 0) {
            sparks.erase(sparks.begin() + i);
        }
    }
}

void drawTree(sf::RenderWindow& window, std::vector<Particle*>& allParticles,
              std::vector<DeletingParticle>& deletingParticles,
              std::vector<Spark>& sparks, int highlightLevel,
              Particle* highlightBranch, float totalTime,
              const sf::Font& font, bool fontLoaded, sf::View& view) {

    window.clear(BACKGROUND_COLOR);
    window.setView(view);

    for (Particle* p : allParticles) {
        if (!p->visible || p->markedForDelete) continue;

        for (Particle* child : p->children) {
            if (child->growProgress <= 0.01f || child->markedForDelete) continue;

            bool highlight = (highlightBranch != nullptr && child->isDescendantOf(highlightBranch));
            sf::Color colorParent = getColor(p->x, p->y, p->level, highlight, totalTime);

            float endX, endY;
            if (child->visible) {
                endX = child->x;
                endY = child->y;
            } else {
                endX = p->x + (child->homeX - p->x) * child->growProgress;
                endY = p->y + (child->homeY - p->y) * child->growProgress;
            }

            sf::Color colorChild = getColor(endX, endY, child->level, highlight, totalTime);

            sf::Vertex line[2];
            line[0].position = sf::Vector2f(p->x, p->y);
            line[0].color = colorParent;
            line[1].position = sf::Vector2f(endX, endY);
            line[1].color = colorChild;

            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
    }

    for (Particle* p : allParticles) {
        if (!p->visible || p->markedForDelete) continue;

        float radius = BASE_PARTICLE_RADIUS - p->level * RADIUS_DECREASE_PER_LEVEL;
        if (radius < MIN_PARTICLE_RADIUS) radius = MIN_PARTICLE_RADIUS;

        sf::CircleShape dot(radius);
        dot.setOrigin(sf::Vector2f(radius, radius));
        dot.setPosition(sf::Vector2f(p->x, p->y));

        bool highlight = (highlightBranch != nullptr && p->isDescendantOf(highlightBranch));
        sf::Color color;

        if (p->level == highlightLevel && !highlight) {
            color = HIGHLIGHT_LEVEL_COLOR;
        } else {
            color = getColor(p->x, p->y, p->level, highlight, totalTime);
        }

        dot.setFillColor(color);
        window.draw(dot);
    }

    for (const DeletingParticle& dp : deletingParticles) {
        float radius = (BASE_PARTICLE_RADIUS - dp.particle->level * RADIUS_DECREASE_PER_LEVEL) * dp.shrinkFactor;
        if (radius < 1) radius = 1;

        sf::CircleShape dot(radius);
        dot.setOrigin(sf::Vector2f(radius, radius));
        dot.setPosition(sf::Vector2f(dp.x, dp.y));

        float flash = sin(dp.deleteProgress * 3.14159f * 10) * 0.5f + 0.5f;
        sf::Color color(
            (int)(dp.originalColor.r * (1-flash) + 255 * flash),
            (int)(dp.originalColor.g * (1-flash) + 50 * flash),
            (int)(dp.originalColor.b * (1-flash) + 50 * flash),
            (int)(dp.originalColor.a * (1.0f - dp.deleteProgress * 0.5f))
        );

        dot.setFillColor(color);
        window.draw(dot);
    }

    for (const Spark& s : sparks) {
        sf::CircleShape spark(SPARK_SIZE);
        spark.setPosition(sf::Vector2f(s.x, s.y));

        int alpha = static_cast<int>(s.life * 255);
        if (alpha < 0) alpha = 0;
        if (alpha > 255) alpha = 255;

        spark.setFillColor(sf::Color(s.r, s.g, s.b, alpha));
        window.draw(spark);
    }

    window.setView(window.getDefaultView());

    if (fontLoaded) {
        sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window), view);

        for (Particle* p : allParticles) {
            if (!p->visible || p->markedForDelete) continue;

            float dx = p->x - mouseWorld.x;
            float dy = p->y - mouseWorld.y;

            if (dx*dx + dy*dy < HIT_DETECTION_RADIUS_SQ) {
                std::stringstream ss;
                ss << "Level: " << p->level << "\n";
                ss << "Properties: " << p->props.size() << "\n\n";
                ss << "Left click: drag/highlight\n";
                ss << "Right click: delete branch\n";
                ss << "Double click: highlight branch\n\n";

                int count = 0;
                for (const auto& prop : p->props) {
                    ss << prop;
                    count++;
                    if (count % 2 == 0) {
                        ss << "\n";
                    } else {
                        ss << " | ";
                    }
                }

                sf::Text infoText(font, ss.str(), TOOLTIP_FONT_SIZE);
                infoText.setFillColor(sf::Color::White);

                sf::FloatRect bounds = infoText.getGlobalBounds();

                sf::RectangleShape background(sf::Vector2f(
                    bounds.size.x + TOOLTIP_BACKGROUND_PADDING,
                    bounds.size.y + TOOLTIP_BACKGROUND_PADDING));

                sf::Vector2f pos = sf::Vector2f(sf::Mouse::getPosition(window)) +
                                   sf::Vector2f(TOOLTIP_OFFSET_X, TOOLTIP_OFFSET_Y);

                if (pos.x + background.getSize().x > WINDOW_WIDTH) {
                    pos.x -= background.getSize().x + TOOLTIP_OFFSET_X * 2;
                }
                if (pos.y + background.getSize().y > WINDOW_HEIGHT) {
                    pos.y -= background.getSize().y + TOOLTIP_OFFSET_Y * 2;
                }

                background.setPosition(pos);
                background.setFillColor(TOOLTIP_BACKGROUND_COLOR);
                infoText.setPosition(pos + sf::Vector2f(TOOLTIP_PADDING, TOOLTIP_PADDING));

                window.draw(background);
                window.draw(infoText);
                break;
            }
        }
    }
}
