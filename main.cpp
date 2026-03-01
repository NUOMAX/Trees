#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace sf;
using namespace std;

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

// Параметры роста и анимации
const float GROW_SPEED_MIN = 0.5f;
const float GROW_SPEED_MAX = 1.21f;
const float HOME_RETURN_FORCE = 4.0f;
const float VELOCITY_DAMPING = 0.82f;

// Параметры отталкивания частиц
const float REPULSION_RADIUS = 6500.0f;
const float REPULSION_FORCE = 1600.0f;

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

// Параметры цвета
const int COLOR_Y_MIN = 50;
const int COLOR_Y_MAX = 850;
const int COLOR_CENTER_X = 640;
const float COLOR_WAVE_FREQUENCY = 0.7f;
const float COLOR_WAVE_AMPLITUDE = 0.15f;

// Параметры подсветки
const int DOUBLE_CLICK_TIME_MS = 300;
const Color HIGHLIGHT_BRANCH_COLOR = Color::Green;
const Color HIGHLIGHT_LEVEL_COLOR = Color::Yellow;

// Параметры искр
const int SPARKS_PER_BIRTH = 15;
const float SPARK_LIFETIME = 1.2f;
const float SPARK_SIZE = 1.2f;
const int SPARK_SPEED_MIN = 60;
const int SPARK_SPEED_MAX = 150;

// Параметры подсказки
const int TOOLTIP_FONT_SIZE = 14;
const int TOOLTIP_OFFSET_X = 20;
const int TOOLTIP_OFFSET_Y = 20;
const int TOOLTIP_PADDING = 6;
const int TOOLTIP_BACKGROUND_PADDING = 12;
const Color TOOLTIP_BACKGROUND_COLOR(0, 0, 0, 220);

// Параметры камеры
const float CAMERA_ZOOM_IN_FACTOR = 0.9f;
const float CAMERA_ZOOM_OUT_FACTOR = 1.1f;

// Параметры окна
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int FRAME_RATE_LIMIT = 60;
const Color BACKGROUND_COLOR(5, 5, 10);

const float MAX_LEVEL_FOR_FULL_PHYSICS = 3;
const float DEEP_LEVEL_DAMPING_BOOST = 1.3f;
const float DEEP_LEVEL_RETURN_REDUCTION = 0.6f;
const float DEEP_LEVEL_ANCHOR_STRENGTH = 0.7f;
const int DEEP_LEVEL_START = 4;

const float DELETE_ANIMATION_DURATION = 1.0f;
const int DELETE_SPARKS_COUNT = 30;
const float DELETE_SHRINK_SPEED = 2.0f;
const Color DELETE_FLASH_COLOR = Color::Red;

const vector<string> EN_PROPS = {
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

mt19937 gen(random_device{}());

struct Spark {
    float x, y;
    float vx, vy;
    int r, g, b;
    float life;
};

struct Particle;

struct DeletingParticle {
    Particle* particle;
    float deleteProgress;
    float x, y;
    float vx, vy;
    float shrinkFactor;
    Color originalColor;

    DeletingParticle(Particle* p, Color color);
};

struct Particle {
    vector<string> props;
    float homeX, homeY;
    float x, y;
    float vx, vy;
    vector<Particle*> children;
    Particle* parent;
    float growProgress;
    float growSpeed;
    bool visible;
    bool dragged;
    int level;
    int weight;
    float stability;
    bool markedForDelete;

    Particle(vector<string> p, int lvl, float startX, float startY, Particle* par) {
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

    ~Particle() {
        for (int i = 0; i < children.size(); i++) {
            delete children[i];
        }
    }

    bool hasProp(string prop) {
        for (int i = 0; i < props.size(); i++) {
            if (props[i] == prop) return true;
        }
        return false;
    }

    bool isDescendantOf(Particle* target) {
        if (target == nullptr) return false;
        if (this == target) return true;
        if (parent == nullptr) return false;
        return parent->isDescendantOf(target);
    }

    void markBranchForDelete() {
        markedForDelete = true;
        for (int i = 0; i < children.size(); i++) {
            children[i]->markBranchForDelete();
        }
    }
};

DeletingParticle::DeletingParticle(Particle* p, Color color) {
    particle = p;
    deleteProgress = 0.0f;
    x = p->x;
    y = p->y;
    vx = p->vx;
    vy = p->vy;
    shrinkFactor = 1.0f;
    originalColor = color;
}

Color getColor(float x, float y, int level, bool highlight, float time) {
    if (highlight) return HIGHLIGHT_BRANCH_COLOR;

    float yFactor = (y - COLOR_Y_MIN) / (COLOR_Y_MAX - COLOR_Y_MIN);
    if (yFactor < 0) yFactor = 0;
    if (yFactor > 1) yFactor = 1;

    float xFactor = abs(x - COLOR_CENTER_X) / COLOR_CENTER_X;
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

    return Color(r, g, b, a);
}

int countWeight(Particle* p) {
    if (p->children.size() == 0) {
        p->weight = 1;
        return 1;
    }

    int sum = 0;
    for (int i = 0; i < p->children.size(); i++) {
        sum = sum + countWeight(p->children[i]);
    }
    p->weight = sum;
    return sum;
}

void arrangeTree(Particle* p, float x, float y, float angle, float spread, float distance) {
    p->homeX = x;
    p->homeY = y;

    if (p->children.size() == 0) return;

    float currentAngle = angle - spread / 2;

    for (int i = 0; i < p->children.size(); i++) {
        Particle* child = p->children[i];

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

        currentAngle = currentAngle + childSpread;
    }
}

void shuffleArray(vector<string>& arr) {
    for (int i = arr.size() - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        string temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void createTree(Particle* parent, int level) {
    if (parent->props.size() <= MIN_PROPERTIES_PER_NODE || level > MAX_TREE_LEVELS) return;

    int childrenCount = MIN_CHILDREN_PER_NODE +
        (rand() % (MAX_CHILDREN_PER_NODE - MIN_CHILDREN_PER_NODE + 1));

    vector<string> propList;
    for (int i = 0; i < parent->props.size(); i++) {
        propList.push_back(parent->props[i]);
    }

    for (int i = 0; i < childrenCount; i++) {
        vector<string> childProps;
        int propCount = MIN_PROPERTIES_PER_NODE +
            (rand() % (propList.size() - MIN_PROPERTIES_PER_NODE));

        shuffleArray(propList);

        for (int j = 0; j < propCount; j++) {
            bool already = false;
            for (int k = 0; k < childProps.size(); k++) {
                if (childProps[k] == propList[j]) {
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

void collectParticles(Particle* p, vector<Particle*>& list) {
    if (p->markedForDelete) return;
    list.push_back(p);
    for (int i = 0; i < p->children.size(); i++) {
        collectParticles(p->children[i], list);
    }
}

void removeMarkedParticles(vector<Particle*>& allParticles) {
    allParticles.erase(
        std::remove_if(allParticles.begin(), allParticles.end(),
            [](Particle* p) { return p->markedForDelete; }),
        allParticles.end()
    );
}

void deleteBranch(Particle* branchRoot, vector<DeletingParticle>& deletingParticles,
                  vector<Particle*>& allParticles, vector<Spark>& sparks, float totalTime) {

    if (branchRoot->parent == nullptr) {
        return;
    }

    vector<Particle*> branchParticles;
    collectParticles(branchRoot, branchParticles);

    for (Particle* p : branchParticles) {
        if (!p->visible) continue;

        Color col = getColor(p->x, p->y, p->level, false, totalTime);

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

int main() {
    srand(time(nullptr));

    RenderWindow window(VideoMode(Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), "Evolution Tree");
    window.setFramerateLimit(FRAME_RATE_LIMIT);

    Font font;
    bool fontLoaded = false;

    if (!fontLoaded) {
        fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
    }
    if (!fontLoaded) {
        fontLoaded = font.openFromFile("C:/Windows/Fonts/arialbd.ttf");
    }
    if (!fontLoaded) {
        fontLoaded = font.openFromFile("C:/Windows/Fonts/times.ttf");
    }
    if (!fontLoaded) {
        fontLoaded = font.openFromFile("C:/Windows/Fonts/verdana.ttf");
    }
    if (!fontLoaded) {
        fontLoaded = font.openFromFile("C:/Windows/Fonts/seguiemj.ttf");
    }

    if (!fontLoaded) {
        fontLoaded = font.openFromFile("arial.ttf");;
    }

    vector<string> rootProps;
    for (int i = 0; i < EN_PROPS.size(); i++) {
        rootProps.push_back(EN_PROPS[i]);
    }
    Particle* root = new Particle(rootProps, 0, ROOT_START_X, ROOT_START_Y, nullptr);
    root->visible = true;

    createTree(root, 0);
    countWeight(root);
    arrangeTree(root, TREE_CENTER_X, TREE_CENTER_Y,
                TREE_INITIAL_ANGLE, TREE_INITIAL_SPREAD, TREE_INITIAL_DISTANCE);

    vector<Particle*> allParticles;
    collectParticles(root, allParticles);

    vector<Spark> sparks;
    vector<DeletingParticle> deletingParticles;

    View view(FloatRect(Vector2f(0, 0), Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT)));

    bool movingCamera = false;
    int lastMouseX, lastMouseY;

    Particle* draggedParticle = nullptr;
    Particle* lastClicked = nullptr;
    int highlightLevel = -1;
    Particle* highlightBranch = nullptr;

    Clock clock;
    Clock globalTime;
    Clock clickTimer;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        float totalTime = globalTime.getElapsedTime().asSeconds();

        while (auto optionalEvent = window.pollEvent()) {
            Event event = *optionalEvent;

            if (event.is<Event::Closed>()) {
                window.close();
            }

            Vector2f mouse = window.mapPixelToCoords(Mouse::getPosition(window), view);

            if (auto mousePressed = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePressed->button == Mouse::Button::Left) {
                    Particle* hit = nullptr;
                    for (int i = 0; i < allParticles.size(); i++) {
                        Particle* p = allParticles[i];
                        if (!p->visible || p->markedForDelete) continue;

                        float dx = p->x - mouse.x;
                        float dy = p->y - mouse.y;
                        float dist = dx*dx + dy*dy;

                        if (dist < HIT_DETECTION_RADIUS_SQ) {
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
                        lastMouseX = Mouse::getPosition(window).x;
                        lastMouseY = Mouse::getPosition(window).y;
                    }
                }

                if (mousePressed->button == Mouse::Button::Right) {
                    Particle* hit = nullptr;
                    for (int i = 0; i < allParticles.size(); i++) {
                        Particle* p = allParticles[i];
                        if (!p->visible || p->markedForDelete) continue;

                        float dx = p->x - mouse.x;
                        float dy = p->y - mouse.y;
                        float dist = dx*dx + dy*dy;

                        if (dist < HIT_DETECTION_RADIUS_SQ) {
                            hit = p;
                            break;
                        }
                    }

                    if (hit != nullptr) {
                        deleteBranch(hit, deletingParticles, allParticles, sparks, totalTime);
                    }
                }
            }

            if (auto mouseReleased = event.getIf<Event::MouseButtonReleased>()) {
                if (draggedParticle != nullptr) {
                    draggedParticle->dragged = false;
                    draggedParticle = nullptr;
                }
                movingCamera = false;
            }

            if (auto scroll = event.getIf<Event::MouseWheelScrolled>()) {
                if (scroll->delta > 0) {
                    view.zoom(CAMERA_ZOOM_IN_FACTOR);
                } else {
                    view.zoom(CAMERA_ZOOM_OUT_FACTOR);
                }
            }
        }

        if (movingCamera) {
            Vector2i mousePos = Mouse::getPosition(window);
            Vector2f oldWorld = window.mapPixelToCoords(Vector2i(lastMouseX, lastMouseY), view);
            Vector2f newWorld = window.mapPixelToCoords(mousePos, view);

            view.move(oldWorld - newWorld);

            lastMouseX = mousePos.x;
            lastMouseY = mousePos.y;
        }

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
                    vector<Particle*>& siblings = dp.particle->parent->children;
                    for (int j = 0; j < siblings.size(); j++) {
                        if (siblings[j] == dp.particle) {
                            delete dp.particle;
                            siblings.erase(siblings.begin() + j);
                            break;
                        }
                    }
                }
                deletingParticles.erase(deletingParticles.begin() + i);
            }
        }

        for (int i = 0; i < allParticles.size(); i++) {
            Particle* p = allParticles[i];
            if (p->markedForDelete) continue;

            if (p->parent != nullptr && p->parent->visible && !p->visible && !p->parent->markedForDelete) {
                p->growProgress = p->growProgress + p->growSpeed * dt;
                if (p->growProgress > 1) p->growProgress = 1;

                p->x = p->parent->x + (p->homeX - p->parent->x) * p->growProgress;
                p->y = p->parent->y + (p->homeY - p->parent->y) * p->growProgress;

                if (p->growProgress >= 1) {
                    p->visible = true;

                    Color col = getColor(p->x, p->y, p->level, false, totalTime);

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
                Vector2f mouse = window.mapPixelToCoords(Mouse::getPosition(window), view);
                p->x = mouse.x;
                p->y = mouse.y;
                p->vx = 0;
                p->vy = 0;
            } else {
                if (p->level <= MAX_LEVEL_FOR_FULL_PHYSICS) {
                    p->vx = p->vx + (p->homeX - p->x) * HOME_RETURN_FORCE * dt;
                    p->vy = p->vy + (p->homeY - p->y) * HOME_RETURN_FORCE * dt;
                } else {
                    float deepLevelFactor = pow(DEEP_LEVEL_RETURN_REDUCTION,
                                               p->level - MAX_LEVEL_FOR_FULL_PHYSICS);
                    p->vx = p->vx + (p->homeX - p->x) * HOME_RETURN_FORCE * dt * deepLevelFactor;
                    p->vy = p->vy + (p->homeY - p->y) * HOME_RETURN_FORCE * dt * deepLevelFactor;
                }

                for (int j = 0; j < allParticles.size(); j++) {
                    Particle* other = allParticles[j];
                    if (p == other) continue;
                    if (!other->visible || other->markedForDelete) continue;

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
                        p->vx = p->vx + dx * force;
                        p->vy = p->vy + dy * force;
                    }
                }

                p->x = p->x + p->vx;
                p->y = p->y + p->vy;

                float damping = VELOCITY_DAMPING;
                if (p->level > MAX_LEVEL_FOR_FULL_PHYSICS) {
                    float boost = pow(DEEP_LEVEL_DAMPING_BOOST,
                                      p->level - MAX_LEVEL_FOR_FULL_PHYSICS);
                    damping = VELOCITY_DAMPING * boost;
                    if (damping > 0.98f) damping = 0.98f;
                }

                p->vx = p->vx * damping;
                p->vy = p->vy * damping;

                if (p->level >= DEEP_LEVEL_START) {
                    p->x = p->x * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                           p->homeX * DEEP_LEVEL_ANCHOR_STRENGTH;
                    p->y = p->y * (1.0f - DEEP_LEVEL_ANCHOR_STRENGTH) +
                           p->homeY * DEEP_LEVEL_ANCHOR_STRENGTH;
                    p->vx = p->vx * 0.5f;
                    p->vy = p->vy * 0.5f;
                }
            }
        }

        for (int i = sparks.size() - 1; i >= 0; i--) {
            sparks[i].x = sparks[i].x + sparks[i].vx * dt;
            sparks[i].y = sparks[i].y + sparks[i].vy * dt;
            sparks[i].life = sparks[i].life - dt;

            if (sparks[i].life <= 0) {
                sparks.erase(sparks.begin() + i);
            }
        }

        window.clear(BACKGROUND_COLOR);
        window.setView(view);

        for (int i = 0; i < allParticles.size(); i++) {
            Particle* p = allParticles[i];
            if (!p->visible || p->markedForDelete) continue;

            for (int j = 0; j < p->children.size(); j++) {
                Particle* child = p->children[j];
                if (child->growProgress <= 0.01f || child->markedForDelete) continue;

                bool highlight = false;
                if (highlightBranch != nullptr && child->isDescendantOf(highlightBranch)) {
                    highlight = true;
                }

                Color colorParent = getColor(p->x, p->y, p->level, highlight, totalTime);

                float endX, endY;
                if (child->visible) {
                    endX = child->x;
                    endY = child->y;
                } else {
                    endX = p->x + (child->homeX - p->x) * child->growProgress;
                    endY = p->y + (child->homeY - p->y) * child->growProgress;
                }

                Color colorChild = getColor(endX, endY, child->level, highlight, totalTime);

                Vertex line[2];
                line[0].position = Vector2f(p->x, p->y);
                line[0].color = colorParent;
                line[1].position = Vector2f(endX, endY);
                line[1].color = colorChild;

                window.draw(line, 2, PrimitiveType::Lines);
            }
        }

        for (int i = 0; i < allParticles.size(); i++) {
            Particle* p = allParticles[i];
            if (!p->visible || p->markedForDelete) continue;

            float radius = BASE_PARTICLE_RADIUS - p->level * RADIUS_DECREASE_PER_LEVEL;
            if (radius < MIN_PARTICLE_RADIUS) radius = MIN_PARTICLE_RADIUS;

            CircleShape dot(radius);

            dot.setOrigin(Vector2f(radius, radius));
            dot.setPosition(Vector2f(p->x, p->y));

            bool highlight = false;
            if (highlightBranch != nullptr && p->isDescendantOf(highlightBranch)) {
                highlight = true;
            }

            Color color;
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

            CircleShape dot(radius);
            dot.setOrigin(Vector2f(radius, radius));
            dot.setPosition(Vector2f(dp.x, dp.y));

            float flash = sin(dp.deleteProgress * 3.14159f * 10) * 0.5f + 0.5f;
            Color color(
                (int)(dp.originalColor.r * (1-flash) + 255 * flash),
                (int)(dp.originalColor.g * (1-flash) + 50 * flash),
                (int)(dp.originalColor.b * (1-flash) + 50 * flash),
                (int)(dp.originalColor.a * (1.0f - dp.deleteProgress * 0.5f))
            );

            dot.setFillColor(color);
            window.draw(dot);
        }

        for (int i = 0; i < sparks.size(); i++) {
            CircleShape spark(SPARK_SIZE);
            spark.setPosition(Vector2f(sparks[i].x, sparks[i].y));

            int alpha = sparks[i].life * 255;
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;

            Color color(sparks[i].r, sparks[i].g, sparks[i].b, alpha);
            spark.setFillColor(color);

            window.draw(spark);
        }

        window.setView(window.getDefaultView());

        if (fontLoaded) {
            Vector2f mouseWorld = window.mapPixelToCoords(Mouse::getPosition(window), view);

            for (int i = 0; i < allParticles.size(); i++) {
                Particle* p = allParticles[i];
                if (!p->visible || p->markedForDelete) continue;

                float dx = p->x - mouseWorld.x;
                float dy = p->y - mouseWorld.y;

                if (dx*dx + dy*dy < HIT_DETECTION_RADIUS_SQ) {
                    stringstream ss;
                    ss << "Level: " << p->level << "\n";
                    ss << "Properties: " << p->props.size() << "\n\n";
                    ss << "Left click: drag/highlight\n";
                    ss << "Right click: delete branch\n";
                    ss << "Double click: highlight branch\n\n";

                    int count = 0;
                    for (int j = 0; j < p->props.size(); j++) {
                        ss << p->props[j];
                        count++;
                        if (count % 2 == 0) {
                            ss << "\n";
                        } else {
                            ss << " | ";
                        }
                    }
                    string text = ss.str();

                    Text infoText(font, text, TOOLTIP_FONT_SIZE);
                    infoText.setFillColor(Color::White);

                    FloatRect bounds = infoText.getGlobalBounds();

                    RectangleShape background(Vector2f(
                        bounds.size.x + TOOLTIP_BACKGROUND_PADDING,
                        bounds.size.y + TOOLTIP_BACKGROUND_PADDING));

                    Vector2f pos = Vector2f(Mouse::getPosition(window)) +
                                   Vector2f(TOOLTIP_OFFSET_X, TOOLTIP_OFFSET_Y);

                    if (pos.x + background.getSize().x > WINDOW_WIDTH) {
                        pos.x = pos.x - background.getSize().x - TOOLTIP_OFFSET_X * 2;
                    }
                    if (pos.y + background.getSize().y > WINDOW_HEIGHT) {
                        pos.y = pos.y - background.getSize().y - TOOLTIP_OFFSET_Y * 2;
                    }

                    background.setPosition(pos);
                    background.setFillColor(TOOLTIP_BACKGROUND_COLOR);

                    infoText.setPosition(pos + Vector2f(TOOLTIP_PADDING, TOOLTIP_PADDING));

                    window.draw(background);
                    window.draw(infoText);

                    break;
                }
            }
        }

        window.display();
    }

    delete root;
    return 0;
}
