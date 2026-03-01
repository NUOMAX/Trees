#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <iostream>

#include "build.h"
#include "animated.h"
#include "windows.h"

int main() {
    srand(time(nullptr));

    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Evolution Tree");
    window.setFramerateLimit(FRAME_RATE_LIMIT);

    sf::Font font = loadFont();
    bool fontLoaded = true;

    std::vector<std::string> rootProps;
    for (const auto& prop : EN_PROPS) {
        rootProps.push_back(prop);
    }

    Particle* root = new Particle(rootProps, 0, ROOT_START_X, ROOT_START_Y, nullptr);
    root->visible = true;

    createTree(root, 0);
    countWeight(root);
    arrangeTree(root, TREE_CENTER_X, TREE_CENTER_Y,
                TREE_INITIAL_ANGLE, TREE_INITIAL_SPREAD, TREE_INITIAL_DISTANCE);

    std::vector<Particle*> allParticles;
    collectParticles(root, allParticles);

    std::vector<Spark> sparks;
    std::vector<DeletingParticle> deletingParticles;

    sf::View view(sf::FloatRect({0, 0}, {WINDOW_WIDTH, WINDOW_HEIGHT}));

    bool movingCamera = false;
    int lastMouseX = 0, lastMouseY = 0;

    Particle* draggedParticle = nullptr;
    Particle* lastClicked = nullptr;
    int highlightLevel = -1;
    Particle* highlightBranch = nullptr;

    sf::Clock clock;
    sf::Clock globalTime;
    sf::Clock clickTimer;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        float totalTime = globalTime.getElapsedTime().asSeconds();

        handleEvents(window, view, allParticles, draggedParticle, lastClicked,
                    highlightLevel, highlightBranch, clickTimer, movingCamera,
                    lastMouseX, lastMouseY, deletingParticles, sparks, totalTime);

        if (movingCamera) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2f oldWorld = window.mapPixelToCoords(sf::Vector2i(lastMouseX, lastMouseY), view);
            sf::Vector2f newWorld = window.mapPixelToCoords(mousePos, view);
            view.move(oldWorld - newWorld);
            lastMouseX = mousePos.x;
            lastMouseY = mousePos.y;
        }

        updateDeletingParticles(deletingParticles, dt);
        updatePhysics(allParticles, dt, sparks, totalTime, window, view);
        updateSparks(sparks, dt);

        drawTree(window, allParticles, deletingParticles, sparks,
                highlightLevel, highlightBranch, totalTime, font, fontLoaded, view);

        window.display();
    }

    delete root;
    return 0;
}
