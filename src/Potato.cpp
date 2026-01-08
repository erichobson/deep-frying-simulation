#include "Potato.h"

#include <algorithm>
#include <cmath>

Potato::Potato(ofVec2f startPos, ofVec2f sz) {
    position = startPos;
    size = sz;
    velocity = ofVec2f(0, 0);

    // Initial raw potato state [2]
    // Raw potato density typically 1.06-1.10 g/cm³
    moistureContent = 0.79f;
    temperature = 20.0f;
    cookedness = 0.0f;
    crustThickness = 0.0f;
    density = 1.08f;
    timeInOil = 0.0f;

    isInOil = false;
    vigorousBubblingPhase = false;

    currentColor = ofColor(230, 215, 170);
}

void Potato::update(float dt, float oilTemp, float oilSurfaceY,
                    float oilDensity, float basketBottomY) {
    if (position.y > oilSurfaceY) {
        if (!isInOil) {
            isInOil = true;
            timeInOil = 0.0f;
        }

        timeInOil += dt;
        vigorousBubblingPhase = (timeInOil < 20.0f);

        // Moisture evaporation [1]
        // Temperature-dependent evaporation following first-order kinetics
        // Vigorous phase (0-20s): higher evaporation rate due to intense
        // boiling Post-vigorous phase (>20s): reduced rate as surface dries
        if (temperature > 100.0f) {
            float evaporationRateBase;
            if (timeInOil < 20.0f) {
                evaporationRateBase =
                    0.02f * dt * (temperature - 100.0f) / 75.0f;
            } else {
                evaporationRateBase =
                    0.015f * dt * (temperature - 100.0f) / 75.0f;
            }

            // Crust acts as barrier, reducing moisture escape by up to 30%
            float effectiveEvaporationRate =
                evaporationRateBase * (1.0f - 0.3f * crustThickness);
            moistureContent -= effectiveEvaporationRate;
            moistureContent = ofClamp(moistureContent, 0.01f, 0.79f);
        }

        // Density model [2]
        // Accounts for water loss, oil uptake, and porosity development
        // Raw: ~1.08 g/cm³ (dense, water-filled cells)
        // Fried: ~0.60 g/cm³ (porous structure with air voids)
        float initial_moisture = 0.79f;
        float progress = 1.0f - (moistureContent / initial_moisture);
        float rho_raw = 1.08f;
        float rho_fried = 0.60f;
        density = rho_raw - ((rho_raw - rho_fried) * progress);
        density = ofClamp(density, rho_fried, rho_raw);

        // Crust formation [3]
        // Two-phase model: rapid initial formation, then stabilization
        float crustFormationCoeff;
        if (timeInOil < 40.0f) {
            crustFormationCoeff = 0.035f;
        } else {
            crustFormationCoeff = 0.010f;
        }
        crustThickness += crustFormationCoeff * dt * (1.0f - crustThickness);
        crustThickness = ofClamp(crustThickness, 0.0f, 1.0f);

        // Heat transfer (Newton's Law of Cooling)
        // dT/dt = h(T_oil - T_potato) where h varies with cooking phase
        float baseTempDiff = oilTemp - temperature;
        float heatTransferCoeff = getEffectiveHeatTransferCoefficient();
        float heatTransferRate = heatTransferCoeff * dt;
        temperature += baseTempDiff * heatTransferRate;
        temperature = ofClamp(temperature, 20.0f, oilTemp);

        // Cookedness based on Maillard reaction kinetics
        // Quadratic progression: k = ((T - 100) / 70)²
        if (temperature > 100.0f && temperature < 170.0f) {
            float tempProgression = (temperature - 100.0f) / 70.0f;
            cookedness = tempProgression * tempProgression;
            cookedness = ofClamp(cookedness, 0.0f, 1.0f);
        } else if (temperature >= 170.0f) {
            cookedness = 1.0f;
        }

        // Buoyancy (Archimedes' principle)
        // F_net = (ρ_potato - ρ_oil) * g * V
        // Raw potato sinks (1.08 > 0.82), cooked potato floats (0.60 < 0.82)
        float densityDiff = density - oilDensity;
        float buoyancyAccel = densityDiff * 800.0f;

        // Viscous drag: F_drag = -c * v
        float dragCoeff = 3.0f;
        float drag = -velocity.y * dragCoeff;

        float netAccel = buoyancyAccel + drag;
        velocity.y += netAccel * dt;

        float terminalVelocity = 150.0f;
        velocity.y = ofClamp(velocity.y, -terminalVelocity, terminalVelocity);

        // Surface behavior when floating
        if (density < oilDensity && position.y < oilSurfaceY + 20.0f) {
            velocity.y *= 0.85f;
            if (position.y < oilSurfaceY + 5.0f) {
                position.y = oilSurfaceY + 5.0f;
                velocity.y = std::max(0.0f, velocity.y);
            }
        }

        // Basket collision
        if (position.y > basketBottomY - size.y / 2.0f) {
            position.y = basketBottomY - size.y / 2.0f;
            velocity.y = -velocity.y * 0.3f;
        }

    } else {
        isInOil = false;
        velocity.y += 600.0f * dt;
    }

    position += velocity * dt;
    currentColor = getCookingColor();
}

void Potato::draw() {
    ofPushMatrix();
    ofTranslate(position.x, position.y);

    float fryHalfWidth = size.x / 2.0f;
    float fryHalfHeight = size.y / 2.0f;
    float cornerRadius = 3.0f;

    // Shadow layer
    ofSetColor(currentColor.r * 0.6f, currentColor.g * 0.55f,
               currentColor.b * 0.5f, 80);
    ofDrawRectRounded(-fryHalfWidth + 2, -fryHalfHeight + 2, size.x, size.y,
                      cornerRadius);

    // Main body gradient
    ofColor bottomColor = currentColor;
    bottomColor.setBrightness(currentColor.getBrightness() * 0.85f);
    ofSetColor(bottomColor);
    ofDrawRectRounded(-fryHalfWidth, 0, size.x, fryHalfHeight, cornerRadius);

    ofSetColor(currentColor);
    ofDrawRectRounded(-fryHalfWidth, -fryHalfHeight, size.x, fryHalfHeight + 2,
                      cornerRadius);

    // Surface texture
    int seed = (int)(position.x * 100 + position.y * 50);
    for (int i = 0; i < 12; i++) {
        float tx = ofNoise(seed + i * 0.3f) * size.x - fryHalfWidth;
        float ty = ofNoise(seed + i * 0.5f + 100) * size.y - fryHalfHeight;
        float tsize = ofNoise(seed + i * 0.7f + 200) * 8 + 3;

        float spotAlpha = 15 + cookedness * 20;
        ofSetColor(currentColor.r - 25, currentColor.g - 30,
                   currentColor.b - 35, spotAlpha);
        ofDrawEllipse(tx, ty, tsize, tsize * 0.7f);
    }

    // Crust rendering
    if (crustThickness > 0.1f) {
        float crustR =
            ofLerp(currentColor.r, currentColor.r - 30, crustThickness);
        float crustG =
            ofLerp(currentColor.g, currentColor.g - 45, crustThickness);
        float crustB =
            ofLerp(currentColor.b, currentColor.b - 55, crustThickness);

        ofNoFill();
        ofSetLineWidth(1.5f + crustThickness * 2.5f);
        ofSetColor(crustR, crustG, crustB, 180 + crustThickness * 60);
        ofDrawRectRounded(-fryHalfWidth, -fryHalfHeight, size.x, size.y,
                          cornerRadius);
        ofFill();

        if (crustThickness > 0.4f) {
            int numBumps = (int)(crustThickness * 20);
            for (int i = 0; i < numBumps; i++) {
                float edgeDist = 0.95f;
                float bx, by;

                if (ofNoise(seed + i * 0.3f) < 0.5f) {
                    bx = (ofNoise(seed + i * 0.4f) * 2 - 1) * fryHalfWidth *
                         edgeDist;
                    by = (ofNoise(seed + i * 0.5f) < 0.5f ? -1 : 1) *
                         fryHalfHeight;
                } else {
                    bx = (ofNoise(seed + i * 0.6f) < 0.5f ? -1 : 1) *
                         fryHalfWidth;
                    by = (ofNoise(seed + i * 0.7f) * 2 - 1) * fryHalfHeight *
                         edgeDist;
                }

                float bumpSize = ofNoise(seed + i * 0.8f) * 3 + 1;
                ofSetColor(crustR - 10, crustG - 15, crustB - 20,
                           100 + crustThickness * 80);
                ofDrawCircle(bx, by, bumpSize);
            }
        }
    }

    // Highlights
    float highlightIntensity = isInOil ? 0.7f : 0.4f;
    ofSetColor(currentColor.r + 60, currentColor.g + 55, currentColor.b + 45,
               100 * highlightIntensity);
    float highlightX = -fryHalfWidth + size.x * 0.08f;
    float highlightY = -fryHalfHeight + size.y * 0.15f;
    float highlightWidth = size.x * 0.55f;
    float highlightHeight = size.y * 0.35f;
    ofDrawRectRounded(highlightX, highlightY, highlightWidth, highlightHeight,
                      2);

    ofSetColor(255, 252, 240, 90 * highlightIntensity);
    ofDrawRectRounded(highlightX + 5, highlightY + 2, highlightWidth * 0.4f,
                      highlightHeight * 0.5f, 1);

    if (isInOil) {
        ofSetColor(255, 240, 200, 40);
        ofDrawRectRounded(-fryHalfWidth + size.x * 0.6f,
                          -fryHalfHeight + size.y * 0.6f, size.x * 0.3f,
                          size.y * 0.25f, 2);
    }

    // Surface bubbling effect
    if (isInOil && vigorousBubblingPhase && moistureContent > 0.1f) {
        int numSurfaceBubbles = (int)(moistureContent * 8);
        for (int i = 0; i < numSurfaceBubbles; i++) {
            float phase = ofGetElapsedTimef() * 3 + i * 1.7f;
            if (sin(phase) > 0.3f) {
                float bx = (ofNoise(seed + i * 1.1f + phase * 0.1f) * 2 - 1) *
                           fryHalfWidth * 0.8f;
                float by = (ofNoise(seed + i * 1.3f + phase * 0.1f) * 2 - 1) *
                           fryHalfHeight * 0.8f;
                float bubbleSize = sin(phase) * 2 + 1;

                ofSetColor(currentColor.r - 20, currentColor.g - 25,
                           currentColor.b - 30, 60);
                ofDrawCircle(bx, by, bubbleSize);
                ofSetColor(255, 250, 230, 40);
                ofDrawCircle(bx - bubbleSize * 0.3f, by - bubbleSize * 0.3f,
                             bubbleSize * 0.4f);
            }
        }
    }

    // Edge outline
    ofNoFill();
    ofSetLineWidth(1.0f);
    ofSetColor(currentColor.r - 40, currentColor.g - 45, currentColor.b - 50,
               60);
    ofDrawRectRounded(-fryHalfWidth, -fryHalfHeight, size.x, size.y,
                      cornerRadius);
    ofFill();

    // Moisture sheen
    if (moistureContent > 0.5f && isInOil) {
        float sheenAlpha = ofMap(moistureContent, 0.5f, 0.79f, 0, 30);
        ofSetColor(255, 255, 255, sheenAlpha);
        ofDrawRectRounded(-fryHalfWidth + 3, -fryHalfHeight + 2, size.x - 6,
                          size.y * 0.4f, 2);
    }

    ofPopMatrix();
}

ofColor Potato::getCookingColor() {
    // Color progression: raw → golden brown
    ofColor raw(235, 220, 175);
    ofColor veryLight(245, 230, 160);
    ofColor light(245, 225, 140);
    ofColor medium(240, 205, 120);
    ofColor golden(220, 180, 100);
    ofColor darkGolden(190, 150, 80);

    if (cookedness < 0.25f) {
        return raw.getLerped(veryLight, cookedness / 0.25f);
    } else if (cookedness < 0.5f) {
        return veryLight.getLerped(light, (cookedness - 0.25f) / 0.25f);
    } else if (cookedness < 0.65f) {
        return light.getLerped(medium, (cookedness - 0.5f) / 0.15f);
    } else if (cookedness < 0.85f) {
        return medium.getLerped(golden, (cookedness - 0.65f) / 0.2f);
    } else {
        return golden.getLerped(darkGolden, (cookedness - 0.85f) / 0.15f);
    }
}

ofVec2f Potato::getSurfacePointForBubble() {
    float x_offset = ofRandom(-size.x / 2.0f, size.x / 2.0f);
    float y_offset = ofRandom(-size.y / 2.0f, size.y / 2.0f);

    // Prefer edges for bubble generation
    if (ofRandom(1.0f) < 0.90f) {
        if (ofRandom(1.0f) < 0.5f) {
            y_offset = ofRandom(1.0f) < 0.5f ? -size.y / 2.0f : size.y / 2.0f;
        } else {
            x_offset = ofRandom(1.0f) < 0.5f ? -size.x / 2.0f : size.x / 2.0f;
        }
    }

    return ofVec2f(position.x + x_offset, position.y + y_offset);
}

float Potato::getBubbleGenerationFactor(float oilTemp) {
    if (!isInOil) return 0.0f;
    if (moistureContent < 0.01f) return 0.0f;

    float tempDiff = oilTemp - temperature;
    if (tempDiff < 5.0f) return 0.0f;

    // Time-based factor: exponential decay during vigorous phase
    float timeBasedFactor;
    if (timeInOil < 20.0f) {
        timeBasedFactor = exp(-timeInOil / 8.0f);
        timeBasedFactor = ofMap(timeBasedFactor, 0.0f, 1.0f, 0.0f, 1.0f, true);
    } else if (timeInOil < 90.0f) {
        timeBasedFactor = ofMap(timeInOil, 20.0f, 90.0f, 1.0f, 0.0f, true);
    } else {
        timeBasedFactor = 0.02f;
    }

    // Moisture factor: quadratic falloff below 10%
    float moistureFactor;
    if (moistureContent > 0.1f) {
        moistureFactor = 1.0f;
    } else if (moistureContent > 0.01f) {
        float moistureRatio = moistureContent / 0.1f;
        moistureFactor = moistureRatio * moistureRatio;
    } else {
        moistureFactor = 0.01f;
    }

    float tempDiffFactor = ofMap(tempDiff, 5.0f, 100.0f, 0.1f, 1.0f, true);

    float baseFactor = moistureFactor * tempDiffFactor * timeBasedFactor;
    baseFactor *= (1.0f - 0.5f * crustThickness);

    if (moistureContent > 0.01f) {
        baseFactor = std::max(baseFactor, 0.01f);
    }

    return baseFactor;
}

float Potato::getEffectiveHeatTransferCoefficient() {
    // Base coefficient calibrated to match real frying dynamics
    // h_eff ≈ 250-500 W/m²K in physical units
    float baseCoeff = 0.025f;

    // Enhanced heat transfer during vigorous boiling phase
    // Bubble agitation increases convective transfer
    if (vigorousBubblingPhase) {
        float bubbleBoost = 1.0f + 4.0f * exp(-timeInOil / 20.0f);
        baseCoeff *= bubbleBoost;
    }

    // Crust acts as thermal barrier
    baseCoeff *= (1.0f - 0.5f * crustThickness);

    return baseCoeff;
}
