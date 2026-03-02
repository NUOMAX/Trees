#include "build.h"
#include "animated.h"
#include "windows.h"

int main() {
    srand(time(nullptr));

    WindowManager windowManager;
    if (!windowManager.isOpen()) return -1;

    std::vector<std::string> rootProps = EN_PROPS;
    Particle* root = new Particle(rootProps, 0,
                                  TreeConfig::ROOT_START_X,
                                  TreeConfig::ROOT_START_Y,
                                  nullptr);
    root->setVisible(true);

    TreeUtils::createTree(root, 0);
    TreeUtils::countWeight(root);
    TreeUtils::arrangeTree(root, TreeConfig::TREE_CENTER_X, TreeConfig::TREE_CENTER_Y,
                          TreeConfig::TREE_INITIAL_ANGLE, TreeConfig::TREE_INITIAL_SPREAD,
                          TreeConfig::TREE_INITIAL_DISTANCE);

    std::vector<Particle*> allParticles;
    TreeUtils::collectParticles(root, allParticles);

    std::vector<Spark> sparks;
    std::vector<DeletingParticle> deletingParticles;

    bool movingCamera = false;
    int lastMouseX = 0, lastMouseY = 0;
    Particle* draggedParticle = nullptr;
    Particle* lastClicked = nullptr;
    int highlightLevel = -1;
    Particle* highlightBranch = nullptr;

    sf::Clock clock;
    sf::Clock globalTime;
    sf::Clock clickTimer;

    while (windowManager.isOpen()) {
        float dt = clock.restart().asSeconds();
        float totalTime = globalTime.getElapsedTime().asSeconds();

        windowManager.pollEvents(allParticles, draggedParticle, lastClicked,
                                highlightLevel, highlightBranch, clickTimer,
                                movingCamera, lastMouseX, lastMouseY,
                                deletingParticles, sparks, totalTime);

        if (movingCamera) {
            sf::Vector2i mousePos = windowManager.getMousePosition();
            sf::Vector2f oldWorld = windowManager.mapPixelToCoords(sf::Vector2i(lastMouseX, lastMouseY));
            sf::Vector2f newWorld = windowManager.mapPixelToCoords(mousePos);
            windowManager.moveCamera(oldWorld - newWorld);
            lastMouseX = mousePos.x;
            lastMouseY = mousePos.y;
        }

        for (auto& dp : deletingParticles) {
            dp.update(dt);
        }

        deletingParticles.erase(
            std::remove_if(deletingParticles.begin(), deletingParticles.end(),
                [](DeletingParticle& dp) {
                    if (dp.isFinished()) {
                        dp.releaseParticle();
                        return true;
                    }
                    return false;
                }),
            deletingParticles.end()
        );

        PhysicsEngine::update(allParticles, dt, sparks, totalTime, windowManager);

        for (int i = sparks.size() - 1; i >= 0; --i) {
            sparks[i].x += sparks[i].vx * dt;
            sparks[i].y += sparks[i].vy * dt;
            sparks[i].life -= dt;

            if (sparks[i].life <= 0) {
                sparks.erase(sparks.begin() + i);
            }
        }

        windowManager.clear();
        windowManager.setView(windowManager.getView());
        Renderer::draw(windowManager.getRenderWindow(), allParticles, deletingParticles, sparks,
                      highlightLevel, highlightBranch, totalTime,
                      windowManager.getFont(), windowManager.isFontLoaded());
        windowManager.display();
    }

    for (auto p : allParticles) {
        delete p;
    }

    return 0;
}
