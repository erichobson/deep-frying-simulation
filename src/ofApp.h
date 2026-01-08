#pragma once

#include "Bubble.h"
#include "Oil.h"
#include "Potato.h"
#include "ofMain.h"

/**
 * Manages the simulation loop, oil thermodynamics, particle systems,
 * user interaction, and scene rendering. Oil properties are computed
 * using physically-based models for density and viscosity.
 *
 * Oil Models:
 *   - Density: Linear thermal expansion ρ(T) = ρ₀ - α(T - T₀)
 *   - Viscosity: Arrhenius temperature dependence μ = A * exp(Ea/RT)
 *
 * Reference:
 *   [4] Fasina, O.O. & Colley, Z. (2008). "Viscosity and specific heat of
 *       vegetable oils as a function of temperature." Int. J. Food Properties,
 *       11(4), 738-746.
 */
class ofApp : public ofBaseApp {
   public:
    void setup();
    ~ofApp();
    void update();
    void draw();

    void keyPressed(int key);
    void mousePressed(int x, int y, int button);
    void mouseDragged(int x, int y, int button);
    void mouseReleased(int x, int y, int button);

   private:
    void updateOilViscosity();
    float getOilDensity();
    void updatePhysics(float dt);
    void spawnBubble(ofVec2f position, float temperature,
                     float depthBelowSurface);

    void drawBackground();
    void drawCountertop();
    void drawFryerHousing();
    void drawFryerContainer();
    void drawFryerBasket();
    void drawOil();
    void drawControlPanel();
    void drawUI();

    float screenWidth;
    float screenHeight;

    float fryerLeftX;
    float fryerRightX;
    float fryerTopY;

    float oilTopY;
    float oilBottomY;

    float basketLeftX;
    float basketRightX;
    float basketTopY;
    float basketBottomY;

    float oilTemperature;
    float targetTemperature;
    float oilViscosity;

    Oil* oilSurface;
    Potato* potatoFry;
    Potato* currentDraggedFry;
    std::vector<Bubble> particles;

    float elapsedTime;
    bool fryInOil;
    bool isPaused;

    ofVec2f dragPosition;
};
