#pragma once

#include "ofMain.h"

/**
 * Simulates the thermodynamic and physical behaviour of a potato during
 * deep frying, including heat transfer, moisture evaporation, density changes,
 * crust formation, and buoyancy dynamics.
 *
 * Physical Models:
 *   - Heat transfer: Newton's Law of Cooling with phase-dependent coefficients
 *   - Density: Linear interpolation from raw (1.08 g/cm³) to fried (0.60 g/cm³)
 *   - Buoyancy: Archimedes' principle with viscous drag
 *   - Cookedness: Maillard reaction kinetics (quadratic temperature
 * progression)
 *
 * References:
 *   [1] Pedreschi, F., et al. (2005). "Modeling water loss during frying
 *       of potato slices." Int. J. Food Properties, 8(2), 289-299.
 *   [2] Costa, R.M., et al. (2008). "Structural changes and shrinkage
 *       of potato during frying." Int. J. Food Sci. Tech., 35(1), 11-23.
 *   [3] Van Koerten, K.N., et al. (2015). "Crust morphology and crispness
 *       development during deep-fat frying." Food Research International, 78,
 *       336-342.
 */
class Potato {
   public:
    Potato(ofVec2f startPos, ofVec2f sz);

    void update(float dt, float oilTemp, float oilSurfaceY, float oilDensity,
                float basketBottomY);
    void draw();

    ofColor getCookingColor();
    ofVec2f getSurfacePointForBubble();
    float getBubbleGenerationFactor(float oilTemp);
    float getEffectiveHeatTransferCoefficient();

    ofVec2f position;
    ofVec2f size;
    ofVec2f velocity;

    float moistureContent;  // [0.01, 0.79] fraction
    float temperature;      // [20, oilTemp] °C
    float cookedness;       // [0, 1] normalized
    float crustThickness;   // [0, 1] normalized
    float density;          // [0.60, 1.08] g/cm³
    float timeInOil;        // seconds

    bool isInOil;
    bool vigorousBubblingPhase;

    ofColor currentColor;
};
