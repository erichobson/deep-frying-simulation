#pragma once

#include "ofMain.h"

/**
 * Provides temperature-based color for oil rendering.
 * Primary oil body rendering is handled by ofApp.
 *
 * Temperature range: 160-190°C (standard deep frying temperatures)
 */
class Oil {
   public:
    Oil(float surfaceY, float initialTemperature);

    void update(float deltaTime);
    ofColor getTemperatureColor();

    float surfaceY;
    float temperature;

   private:
    float time;
};
