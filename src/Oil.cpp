#include "Oil.h"

Oil::Oil(float y, float temp) {
    surfaceY = y;
    temperature = temp;
    time = 0;
}

void Oil::update(float deltaTime) { time += deltaTime; }

ofColor Oil::getTemperatureColor() {
    ofColor coolOil(210, 170, 70, 180);
    ofColor mediumOil(230, 185, 85, 190);
    ofColor hotOil(245, 200, 100, 200);

    float t = ofMap(temperature, 160, 190, 0, 1, true);
    if (t < 0.5f) {
        return coolOil.getLerped(mediumOil, t / 0.5f);
    } else {
        return mediumOil.getLerped(hotOil, (t - 0.5f) / 0.5f);
    }
}
