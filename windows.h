#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

#include "build.h"
#include "animated.h"

namespace WindowConfig {
    const int WIDTH = 1280;
    const int HEIGHT = 720;
    const int FRAME_RATE_LIMIT = 60;
    const sf::Color BACKGROUND_COLOR(5, 5, 10);

    // Параметры камеры
    const float CAMERA_ZOOM_IN_FACTOR = 0.9f;
    const float CAMERA_ZOOM_OUT_FACTOR = 1.1f;

    // Параметры подсказок
    const int TOOLTIP_FONT_SIZE = 14;
    const int TOOLTIP_OFFSET_X = 20;
    const int TOOLTIP_OFFSET_Y = 20;
    const int TOOLTIP_PADDING = 6;
    const int TOOLTIP_BACKGROUND_PADDING = 12;
    const sf::Color TOOLTIP_BACKGROUND_COLOR(0, 0, 0, 220);

    // Параметры двойного клика
    const int DOUBLE_CLICK_TIME_MS = 300;
}

class WindowManager {
private:
    sf::RenderWindow m_window;
    sf::View m_view;
    sf::Font m_font;
    bool m_fontLoaded = false;

public:
    WindowManager();

    sf::RenderWindow& getRenderWindow() { return m_window; }
    bool isOpen() const { return m_window.isOpen(); }
    sf::Font& getFont() { return m_font; }
    bool isFontLoaded() const { return m_fontLoaded; }
    sf::View& getView() { return m_view; }

    void close() { m_window.close(); }
    void clear() { m_window.clear(WindowConfig::BACKGROUND_COLOR); }
    void display() { m_window.display(); }
    void setView(const sf::View& view) { m_window.setView(view); }
    void setDefaultView() { m_window.setView(m_window.getDefaultView()); }

    sf::Vector2f mapPixelToCoords(const sf::Vector2i& pixel) const {
        return m_window.mapPixelToCoords(pixel, m_view);
    }

    sf::Vector2i getMousePosition() const {
        return sf::Mouse::getPosition(m_window);
    }

    void pollEvents(std::vector<Particle*>& allParticles,
                    Particle*& draggedParticle, Particle*& lastClicked,
                    int& highlightLevel, Particle*& highlightBranch,
                    sf::Clock& clickTimer, bool& movingCamera,
                    int& lastMouseX, int& lastMouseY,
                    std::vector<DeletingParticle>& deletingParticles,
                    std::vector<Spark>& sparks, float totalTime);

    void applyZoom(float delta);
    void moveCamera(const sf::Vector2f& delta);
};

class PhysicsEngine {
public:
    static void update(std::vector<Particle*>& allParticles,
                       float dt, std::vector<Spark>& sparks,
                       float totalTime, WindowManager& windowManager);
};

class Renderer {
public:
    static void draw(sf::RenderWindow& window,
                     const std::vector<Particle*>& allParticles,
                     const std::vector<DeletingParticle>& deletingParticles,
                     const std::vector<Spark>& sparks,
                     int highlightLevel, Particle* highlightBranch,
                     float totalTime, const sf::Font& font, bool fontLoaded);
};

#endif // WINDOWS_H_INCLUDED
