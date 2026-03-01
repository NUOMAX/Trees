#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "build.h"
#include "animated.h"

// Параметры окна
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int FRAME_RATE_LIMIT = 60;
const sf::Color BACKGROUND_COLOR(5, 5, 10);

// Параметры камеры
const float CAMERA_ZOOM_IN_FACTOR = 0.9f;
const float CAMERA_ZOOM_OUT_FACTOR = 1.1f;

// Параметры подсказки
const int TOOLTIP_FONT_SIZE = 14;
const int TOOLTIP_OFFSET_X = 20;
const int TOOLTIP_OFFSET_Y = 20;
const int TOOLTIP_PADDING = 6;
const int TOOLTIP_BACKGROUND_PADDING = 12;
const sf::Color TOOLTIP_BACKGROUND_COLOR(0, 0, 0, 220);

// Параметры подсветки
const int DOUBLE_CLICK_TIME_MS = 300;

sf::Font loadFont();
void handleEvents(sf::RenderWindow& window, sf::View& view,
                  std::vector<Particle*>& allParticles,
                  Particle*& draggedParticle, Particle*& lastClicked,
                  int& highlightLevel, Particle*& highlightBranch,
                  sf::Clock& clickTimer, bool& movingCamera,
                  int& lastMouseX, int& lastMouseY,
                  std::vector<DeletingParticle>& deletingParticles,
                  std::vector<Spark>& sparks, float totalTime);

void updatePhysics(std::vector<Particle*>& allParticles, float dt,
                   std::vector<Spark>& sparks, float totalTime,
                   sf::RenderWindow& window, sf::View& view);

void updateDeletingParticles(std::vector<DeletingParticle>& deletingParticles, float dt);
void updateSparks(std::vector<Spark>& sparks, float dt);

void drawTree(sf::RenderWindow& window, std::vector<Particle*>& allParticles,
              std::vector<DeletingParticle>& deletingParticles,
              std::vector<Spark>& sparks, int highlightLevel,
              Particle* highlightBranch, float totalTime,
              const sf::Font& font, bool fontLoaded, sf::View& view);

#endif // WINDOWS_H_INCLUDED
