#include "ofApp.h"

#include <algorithm>
#include <cmath>

ofApp::~ofApp() {
    delete oilSurface;
    delete potatoFry;
}

void ofApp::setup() {
    ofSetFrameRate(60);
    screenWidth = ofGetWidth();
    screenHeight = ofGetHeight();

    // Fryer
    fryerLeftX = (screenWidth * 0.5f) - ((screenWidth * 0.5f) / 2);
    fryerRightX = (screenWidth * 0.5f) + ((screenWidth * 0.5f) / 2);
    fryerTopY = 280.0f;

    // Oil
    oilBottomY = fryerTopY + (screenHeight * 0.35f);
    oilTopY = fryerTopY + 35;

    // Basket
    basketLeftX = fryerLeftX + 40;
    basketRightX = fryerRightX - 40;
    basketBottomY = oilBottomY - 40;
    basketTopY = oilTopY + 30;

    oilTemperature = 175.0f;
    targetTemperature = 175.0f;
    updateOilViscosity();

    oilSurface = new Oil(oilTopY, oilTemperature);
    elapsedTime = 0;
    potatoFry = nullptr;
    fryInOil = false;
    currentDraggedFry = nullptr;
    isPaused = false;
}

void ofApp::updateOilViscosity() {
    // Arrhenius viscosity model [4]
    // μ = A * exp(Ea/RT), non-linear temperature dependence
    float T_Kelvin = oilTemperature + 273.15f;
    float viscosity_inf = 0.00001f;
    float Ea_R = 2500.0f;
    oilViscosity = viscosity_inf * exp(Ea_R / T_Kelvin);
    oilViscosity = ofClamp(oilViscosity, 0.003f, 0.030f);
}

float ofApp::getOilDensity() {
    // Linear thermal expansion model [4]
    // ρ(T) = ρ₀ - α(T - T₀), where ρ₀ = 0.915 g/cm³ at T₀ = 20°C
    // Result: ~0.825 g/cm³ at 160°C, ~0.806 g/cm³ at 190°C
    return 0.915f - 0.00064f * (oilTemperature - 20.0f);
}

void ofApp::update() {
    // Skip all updates when paused
    if (isPaused) return;

    float deltaTime = ofClamp(ofGetLastFrameTime(), 0, 0.1f);
    elapsedTime += deltaTime;

    // Temperature control with exponential smoothing
    oilTemperature += (targetTemperature - oilTemperature) * 0.05f;
    oilTemperature = ofClamp(oilTemperature, 160.0f, 190.0f);
    oilSurface->temperature = oilTemperature;

    updateOilViscosity();

    // Fry physics update
    if (potatoFry != nullptr && fryInOil) {
        float oilDensity = getOilDensity();
        potatoFry->update(deltaTime, oilTemperature, oilTopY, oilDensity,
                          basketBottomY);

        // Override movement when dragging
        if (currentDraggedFry != nullptr) {
            currentDraggedFry->position = dragPosition;
            currentDraggedFry->velocity = ofVec2f(0, 0);
        }

        // Bubble generation
        float bubbleGenerationFactor =
            potatoFry->getBubbleGenerationFactor(oilTemperature);

        if (bubbleGenerationFactor > 0.0f) {
            float minBubblesTarget = 0.5f;
            float maxBubblesTarget = 20.0f;
            float targetNumBubbles =
                ofMap(bubbleGenerationFactor, 0.0f, 1.0f, minBubblesTarget,
                      maxBubblesTarget, true);

            int numBubbles =
                (int)ofRandom(std::max(0.0f, targetNumBubbles - 3.0f),
                              targetNumBubbles + 3.0f);
            numBubbles = ofClamp(numBubbles, 0, (int)maxBubblesTarget);

            // Sporadic generation at low rates
            if (numBubbles < 2 &&
                ofRandom(1.0f) > bubbleGenerationFactor * 8.0f) {
                numBubbles = 0;
            }

            for (int i = 0; i < numBubbles; i++) {
                ofVec2f bubblePos = potatoFry->getSurfacePointForBubble();
                bubblePos.y = ofClamp(bubblePos.y, oilTopY + 5, oilBottomY - 5);
                float depthBelowSurface = bubblePos.y - oilTopY;
                spawnBubble(bubblePos, oilTemperature, depthBelowSurface);
            }
        }
    }

    updatePhysics(deltaTime);
    oilSurface->update(deltaTime);

    // Remove dead particles
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](Bubble& p) { return p.isDead; }),
                    particles.end());
}

void ofApp::updatePhysics(float dt) {
    float oilLeft = fryerLeftX + 15;
    float oilRight = fryerRightX - 15;

    for (auto& p : particles) {
        p.update(dt, oilViscosity);

        // Boundary constraints
        if (p.position.x < oilLeft) p.position.x = oilLeft;
        if (p.position.x > oilRight) p.position.x = oilRight;
    }
}

void ofApp::spawnBubble(ofVec2f position, float temperature,
                        float depthBelowSurface) {
    particles.push_back(
        Bubble(position, temperature, depthBelowSurface, oilTopY));
}

void ofApp::draw() {
    drawBackground();
    drawCountertop();
    drawFryerHousing();
    drawFryerContainer();
    drawOil();

    if (potatoFry != nullptr) {
        potatoFry->draw();
    }

    for (auto& p : particles) {
        p.draw();
    }

    drawFryerBasket();
    drawControlPanel();
    drawUI();
}

void ofApp::drawOil() {
    float oilLeft = fryerLeftX + 15;
    float oilRight = fryerRightX - 15;

    ofColor baseColor = oilSurface->getTemperatureColor();
    float tempFactor = ofMap(oilTemperature, 160, 190, 0, 1, true);

    ofColor surfaceColor = baseColor;
    surfaceColor.r = ofClamp(surfaceColor.r + tempFactor * 15, 0, 255);

    ofColor midColor = baseColor;
    midColor.setBrightness(baseColor.getBrightness() * 0.72f);
    midColor.setSaturation(baseColor.getSaturation() * 1.15f);

    ofColor deepColor = baseColor;
    deepColor.setBrightness(baseColor.getBrightness() * 0.55f);
    deepColor.setSaturation(baseColor.getSaturation() * 1.25f);

    ofColor bottomColor(deepColor.r * 0.75f, deepColor.g * 0.68f,
                        deepColor.b * 0.55f, deepColor.a);

    int segments = 30;
    float depth1 = oilTopY + (oilBottomY - oilTopY) * 0.25f;
    float depth2 = oilTopY + (oilBottomY - oilTopY) * 0.55f;
    float depth3 = oilTopY + (oilBottomY - oilTopY) * 0.8f;

    // Layer 1: Surface to shallow
    ofMesh layer1;
    layer1.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float x = ofMap(i, 0, segments, oilLeft, oilRight);
        float surfaceWave = ofNoise(x * 0.008f, elapsedTime * 0.4f) * 4;
        surfaceWave += ofNoise(x * 0.02f, elapsedTime * 0.8f) * 2;

        layer1.addVertex(ofVec3f(x, oilTopY + surfaceWave, 0));
        layer1.addColor(surfaceColor);
        layer1.addVertex(ofVec3f(x, depth1, 0));
        layer1.addColor(midColor);
    }
    layer1.draw();

    // Layer 2: Shallow to mid
    ofMesh layer2;
    layer2.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float x = ofMap(i, 0, segments, oilLeft, oilRight);
        layer2.addVertex(ofVec3f(x, depth1, 0));
        layer2.addColor(midColor);
        layer2.addVertex(ofVec3f(x, depth2, 0));
        layer2.addColor(deepColor);
    }
    layer2.draw();

    // Layer 3: Mid to deep
    ofMesh layer3;
    layer3.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float x = ofMap(i, 0, segments, oilLeft, oilRight);
        layer3.addVertex(ofVec3f(x, depth2, 0));
        layer3.addColor(deepColor);
        layer3.addVertex(ofVec3f(x, depth3, 0));
        layer3.addColor(bottomColor);
    }
    layer3.draw();

    // Layer 4: Deep to bottom
    ofMesh layer4;
    layer4.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    ofColor floorColor = bottomColor;
    floorColor.setBrightness(bottomColor.getBrightness() * 0.7f);
    for (int i = 0; i <= segments; i++) {
        float x = ofMap(i, 0, segments, oilLeft, oilRight);
        layer4.addVertex(ofVec3f(x, depth3, 0));
        layer4.addColor(bottomColor);
        layer4.addVertex(ofVec3f(x, oilBottomY, 0));
        layer4.addColor(floorColor);
    }
    layer4.draw();

    // Subsurface scattering effect
    float scatterIntensity = ofMap(oilTemperature, 160, 190, 0.15f, 0.4f);
    ofSetColor(255, 200, 100, 12 * scatterIntensity);
    for (int i = 0; i < 5; i++) {
        float scatterY = oilTopY + (oilBottomY - oilTopY) * (0.15f + i * 0.15f);
        float scatterWidth = (oilRight - oilLeft) * (0.9f - i * 0.12f);
        float centerX = (oilLeft + oilRight) / 2;
        float waveOffset = sin(elapsedTime * 0.3f + i) * 5;
        ofDrawEllipse(centerX + waveOffset, scatterY, scatterWidth, 35 - i * 4);
    }

    // Convection currents
    ofSetLineWidth(1.5f);
    int numCurrents = 4;
    for (int c = 0; c < numCurrents; c++) {
        float baseX = ofMap(c, 0, numCurrents - 1, oilLeft + 40, oilRight - 40);
        float phase = elapsedTime * 0.2f + c * 1.5f;

        ofBeginShape();
        ofNoFill();
        for (int i = 0; i < 12; i++) {
            float t = (float)i / 11;
            float y = ofMap(t, 0, 1, oilBottomY - 20, oilTopY + 30);
            float xOffset =
                sin(t * PI * 2 + phase) * 25 + ofNoise(y * 0.01f, phase) * 15;
            float alpha = sin(t * PI) * 8 * scatterIntensity;
            ofSetColor(255, 220, 150, alpha);
            ofVertex(baseX + xOffset, y);
        }
        ofEndShape();
        ofFill();
    }

    // Shimmer effects
    float shimmerIntensity = ofMap(oilTemperature, 160, 190, 0.3f, 1.0f);
    int numShimmers = (int)(8 * shimmerIntensity);
    for (int i = 0; i < numShimmers; i++) {
        float shimmerX = oilLeft + ofNoise(i * 0.5f, elapsedTime * 0.1f) *
                                       (oilRight - oilLeft);
        float shimmerY =
            oilTopY +
            ofNoise(i * 0.7f + 100, elapsedTime * 0.15f) * (depth2 - oilTopY);
        float shimmerPhase = sin(elapsedTime * 3 + i * 2.1f);

        if (shimmerPhase > 0.7f) {
            float shimmerAlpha = ofMap(shimmerPhase, 0.7f, 1.0f, 0, 60);
            float shimmerSize = ofMap(shimmerPhase, 0.7f, 1.0f, 2, 5);
            ofSetColor(255, 250, 220, shimmerAlpha);
            ofDrawCircle(shimmerX, shimmerY, shimmerSize);
            ofSetColor(255, 255, 240, shimmerAlpha * 0.7f);
            ofDrawCircle(shimmerX, shimmerY, shimmerSize * 0.4f);
        }
    }

    // Depth bands
    for (int band = 0; band < 4; band++) {
        float bandY = oilTopY + (oilBottomY - oilTopY) * (0.2f + band * 0.2f);
        float bandAlpha = ofMap(band, 0, 3, 8, 3);

        ofBeginShape();
        ofNoFill();
        ofSetLineWidth(1.0f);
        ofSetColor(0, 0, 0, bandAlpha);
        for (float x = oilLeft; x <= oilRight; x += 8) {
            float waveOffset =
                ofNoise(x * 0.015f, bandY * 0.008f, elapsedTime * 0.15f) * 6;
            ofVertex(x, bandY + waveOffset);
        }
        ofEndShape();
        ofFill();
    }

    // Surface film
    ofSetColor(255, 245, 200, 15);
    ofBeginShape();
    for (float x = oilLeft; x <= oilRight; x += 6) {
        float surfaceWave = ofNoise(x * 0.008f, elapsedTime * 0.4f) * 4;
        surfaceWave += ofNoise(x * 0.02f, elapsedTime * 0.8f) * 2;
        ofVertex(x, oilTopY + surfaceWave);
    }
    for (float x = oilRight; x >= oilLeft; x -= 6) {
        float surfaceWave = ofNoise(x * 0.008f, elapsedTime * 0.4f) * 4;
        surfaceWave += ofNoise(x * 0.02f, elapsedTime * 0.8f) * 2;
        ofVertex(x, oilTopY + surfaceWave + 8);
    }
    ofEndShape(true);
}

void ofApp::drawUI() {
    float lineHeight = 14;
    float panelY = 10;
    float panelHeight = 125;
    float colWidth = (screenWidth - 40) / 3;

    // Panel
    ofSetColor(30, 35, 40, 230);
    ofDrawRectRounded(10, panelY, screenWidth - 20, panelHeight, 6);

    // Border
    ofSetColor(80, 85, 90, 180);
    ofSetLineWidth(1.5f);
    ofNoFill();
    ofDrawRectRounded(10, panelY, screenWidth - 20, panelHeight, 6);
    ofFill();

    // Title
    ofSetColor(255, 200, 100, 255);
    string title = "DEEP-FRYING SIMULATION";
    float titleX = (screenWidth - title.length() * 8) / 2;
    ofDrawBitmapString(title, titleX, panelY + 16);

    // Column 1: Controls
    float col1X = 20;
    float colStartY = panelY + 32;
    float currentY = colStartY;

    ofSetColor(180, 185, 190, 255);
    ofDrawBitmapString("CONTROLS", col1X, currentY);
    currentY += lineHeight + 3;

    ofSetColor(140, 145, 150, 220);
    ofDrawBitmapString("[UP/DOWN] Temp", col1X, currentY);
    currentY += lineHeight;
    ofDrawBitmapString("[SPACE]   Drop/Remove", col1X, currentY);
    currentY += lineHeight;
    ofDrawBitmapString("[P]       Pause", col1X, currentY);
    currentY += lineHeight;
    ofDrawBitmapString("[R]       Reset", col1X, currentY);
    currentY += lineHeight;
    ofDrawBitmapString("[MOUSE]   Drag", col1X, currentY);

    // Column 2: Oil Properties
    float col2X = col1X + colWidth + 10;
    currentY = colStartY;

    ofSetColor(180, 185, 190, 255);
    ofDrawBitmapString("OIL", col2X, currentY);
    currentY += lineHeight + 3;

    // Oil temperature
    float tempNormalized = ofMap(oilTemperature, 160, 190, 0, 1, true);
    ofColor tempColor =
        ofColor(100, 180, 255).getLerped(ofColor(255, 100, 50), tempNormalized);
    ofSetColor(tempColor);
    string tempStr = "Temp: " + ofToString(oilTemperature, 1) + " C";
    if (abs(targetTemperature - oilTemperature) > 0.5f) {
        tempStr += (targetTemperature > oilTemperature) ? " ^" : " v";
    }
    ofDrawBitmapString(tempStr, col2X, currentY);
    currentY += lineHeight;

    // Oil density
    float oilDensity = getOilDensity();
    ofSetColor(140, 200, 180, 240);
    ofDrawBitmapString("Density: " + ofToString(oilDensity, 3) + " g/cm3",
                       col2X, currentY);
    currentY += lineHeight;

    // Formulas
    currentY += 4;
    ofSetColor(100, 105, 110, 180);
    ofDrawBitmapString("p = 0.915 - 0.00064(T-20)", col2X, currentY);

    // Column 3: Fry Status
    float col3X = col2X + colWidth;
    currentY = colStartY;

    ofSetColor(180, 185, 190, 255);
    ofDrawBitmapString("FRY", col3X, currentY);
    currentY += lineHeight + 3;

    if (potatoFry != nullptr) {
        float oilDens = getOilDensity();
        float fryDens = potatoFry->density;
        bool isFloating = fryDens < oilDens;

        // Fry temperature
        float heatTransfer = oilTemperature - potatoFry->temperature;
        float fryTempNorm = ofMap(potatoFry->temperature, 20, 170, 0, 1, true);
        ofColor fryTempColor =
            ofColor(100, 180, 255)
                .getLerped(ofColor(255, 180, 80), fryTempNorm);
        ofSetColor(fryTempColor);
        string fryTempStr =
            "Temp: " + ofToString(potatoFry->temperature, 1) + " C";
        if (heatTransfer > 5) fryTempStr += " ^";
        ofDrawBitmapString(fryTempStr, col3X, currentY);
        currentY += lineHeight;

        // Fry density with buoyancy indicator
        ofColor densColor;
        string buoyancyStr;
        if (isFloating) {
            densColor = ofColor(100, 220, 140, 240);
            buoyancyStr = " [FLOAT]";
        } else {
            densColor = ofColor(220, 140, 100, 240);
            buoyancyStr = " [SINK]";
        }
        ofSetColor(densColor);
        ofDrawBitmapString("Density: " + ofToString(fryDens, 3) + buoyancyStr,
                           col3X, currentY);
        currentY += lineHeight;

        // Moisture with evaporation indicator
        ofSetColor(100, 180, 220, 240);
        string moistureStr =
            "H2O: " + ofToString(potatoFry->moistureContent * 100, 0) + "%";
        if (potatoFry->temperature > 100 &&
            potatoFry->moistureContent > 0.05f) {
            moistureStr += " [EVAP]";
        }
        ofDrawBitmapString(moistureStr, col3X, currentY);
        currentY += lineHeight;

        // Cookedness with progress indicator
        float cookedPct = potatoFry->cookedness * 100;
        float cookedNorm = potatoFry->cookedness;
        ofColor cookedColor =
            ofColor(180, 180, 180)
                .getLerped(ofColor(220, 180, 100), cookedNorm);
        ofSetColor(cookedColor);
        string cookedStr = "Cooked: " + ofToString(cookedPct, 0) + "%";
        if (cookedPct >= 70) cookedStr += " [DONE]";
        ofDrawBitmapString(cookedStr, col3X, currentY);
        currentY += lineHeight;

        // Crust and time
        ofSetColor(220, 180, 120, 220);
        ofDrawBitmapString(
            "Crust: " + ofToString(potatoFry->crustThickness * 100, 0) +
                "%  t=" + ofToString(potatoFry->timeInOil, 1) + "s",
            col3X, currentY);
    } else {
        ofSetColor(120, 125, 130, 200);
        ofDrawBitmapString("No fry in oil", col3X, currentY);
        currentY += lineHeight;
        ofSetColor(100, 105, 110, 160);
        ofDrawBitmapString("Press SPACE to drop", col3X, currentY);
    }

    // Paused indicator
    if (isPaused) {
        string pauseText = "PAUSED";
        float pauseX = screenWidth - pauseText.length() * 8 - 20;
        float pauseY = screenHeight - 20;

        ofSetColor(0, 0, 0, 150);
        ofDrawRectRounded(pauseX - 8, pauseY - 12, pauseText.length() * 8 + 16,
                          18, 3);

        ofSetColor(255, 200, 100, 230);
        ofDrawBitmapString(pauseText, pauseX, pauseY);
    }
}

void ofApp::keyPressed(int key) {
    if (key == 'p' || key == 'P') {
        isPaused = !isPaused;
    } else if (key == OF_KEY_UP) {
        targetTemperature = ofClamp(targetTemperature + 5, 160, 190);
    } else if (key == OF_KEY_DOWN) {
        targetTemperature = ofClamp(targetTemperature - 5, 160, 190);
    } else if (key == ' ') {
        if (fryInOil) {
            if (potatoFry != nullptr) {
                delete potatoFry;
                potatoFry = nullptr;
            }
            fryInOil = false;
        } else {
            // Spawn fry above oil surface
            // Raw potato (1.08 g/cm³) sinks in oil (~0.82 g/cm³)
            ofVec2f fryPos(screenWidth / 2, oilTopY - 80);
            potatoFry = new Potato(fryPos, ofVec2f(120, 20));
            potatoFry->velocity = ofVec2f(0, 100.0f);
            fryInOil = true;
        }
    } else if (key == 'r' || key == 'R') {
        if (potatoFry != nullptr) {
            delete potatoFry;
            potatoFry = nullptr;
        }
        fryInOil = false;
        elapsedTime = 0;
        particles.clear();
    }
}

void ofApp::mousePressed(int x, int y, int button) {
    if (potatoFry != nullptr) {
        float dx = x - potatoFry->position.x;
        float dy = y - potatoFry->position.y;
        if (sqrt(dx * dx + dy * dy) < 60) {
            currentDraggedFry = potatoFry;
            dragPosition = ofVec2f(x, y);
        }
    }
}

void ofApp::mouseDragged(int x, int y, int button) {
    if (currentDraggedFry != nullptr) {
        dragPosition = ofVec2f(x, y);
    }
}

void ofApp::mouseReleased(int x, int y, int button) {
    currentDraggedFry = nullptr;
}

void ofApp::drawFryerContainer() {
    float wallThickness = 15;

    // Left wall
    ofMesh leftWall;
    leftWall.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    for (int i = 0; i <= 12; i++) {
        float y = ofMap(i, 0, 12, fryerTopY, oilBottomY);
        float progress = (float)i / 12;

        ofColor wallTop(130, 135, 140);
        ofColor wallBottom(105, 110, 115);
        ofColor c = wallTop.getLerped(wallBottom, progress);

        leftWall.addVertex(ofVec3f(fryerLeftX, y, 0));
        leftWall.addColor(c);
        leftWall.addVertex(ofVec3f(fryerLeftX + wallThickness, y, 0));
        leftWall.addColor(c.getLerped(ofColor(140, 145, 150), 0.3f));
    }
    leftWall.draw();

    // Right wall
    ofMesh rightWall;
    rightWall.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    for (int i = 0; i <= 12; i++) {
        float y = ofMap(i, 0, 12, fryerTopY, oilBottomY);
        float progress = (float)i / 12;

        ofColor wallTop(130, 135, 140);
        ofColor wallBottom(105, 110, 115);
        ofColor c = wallTop.getLerped(wallBottom, progress);

        rightWall.addVertex(ofVec3f(fryerRightX - wallThickness, y, 0));
        rightWall.addColor(c.getLerped(ofColor(140, 145, 150), 0.3f));
        rightWall.addVertex(ofVec3f(fryerRightX, y, 0));
        rightWall.addColor(c);
    }
    rightWall.draw();

    // Bottom
    ofSetColor(95, 100, 105, 255);
    ofDrawRectangle(fryerLeftX, oilBottomY, fryerRightX - fryerLeftX,
                    wallThickness);

    // Top
    ofSetColor(150, 155, 160, 230);
    ofDrawRectangle(fryerLeftX, fryerTopY - 5, fryerRightX - fryerLeftX, 5);

    ofSetColor(180, 185, 190, 180);
    ofSetLineWidth(2);
    ofDrawLine(fryerLeftX, fryerTopY, fryerRightX, fryerTopY);
}

void ofApp::drawBackground() {
    ofMesh bgMesh;
    bgMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    int steps = 10;
    for (int i = 0; i <= steps; i++) {
        float y = ofMap(i, 0, steps, 0, screenHeight);
        ofColor c = ofColor(200, 205, 210)
                        .getLerped(ofColor(180, 185, 190), (float)i / steps);
        bgMesh.addVertex(ofVec3f(0, y, 0));
        bgMesh.addColor(c);
        bgMesh.addVertex(ofVec3f(screenWidth, y, 0));
        bgMesh.addColor(c);
    }
    bgMesh.draw();

    ofSetColor(220, 225, 230, 40);
    float glowCenterX = screenWidth / 2;
    float glowWidth = 400;
    for (int i = 0; i < 8; i++) {
        float alpha = 15 - i * 2;
        ofSetColor(255, 255, 255, alpha);
        ofDrawEllipse(glowCenterX, screenHeight * 0.45f, glowWidth + i * 40,
                      400 + i * 30);
    }
}

void ofApp::drawCountertop() {
    float countertopY = oilBottomY + 15;

    ofMesh counterMesh;
    counterMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    ofColor topColor(165, 170, 175, 255);
    ofColor bottomColor(130, 135, 140, 255);

    counterMesh.addVertex(ofVec3f(0, countertopY, 0));
    counterMesh.addColor(topColor);
    counterMesh.addVertex(ofVec3f(screenWidth, countertopY, 0));
    counterMesh.addColor(topColor);
    counterMesh.addVertex(ofVec3f(0, screenHeight, 0));
    counterMesh.addColor(bottomColor);
    counterMesh.addVertex(ofVec3f(screenWidth, screenHeight, 0));
    counterMesh.addColor(bottomColor);
    counterMesh.draw();

    ofSetColor(190, 195, 200, 200);
    ofSetLineWidth(4);
    ofDrawLine(0, countertopY, screenWidth, countertopY);
}

void ofApp::drawFryerHousing() {
    float housingLeft = fryerLeftX - 30;
    float housingRight = fryerRightX + 30;
    float housingTop = fryerTopY - 15;
    float housingBottom = oilBottomY + 80;

    ofMesh backWall;
    backWall.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

    int segments = 10;
    for (int i = 0; i <= segments; i++) {
        float y = ofMap(i, 0, segments, housingTop, housingBottom);
        float progress = (float)i / segments;

        ofColor topColor(140, 145, 150);
        ofColor bottomColor(110, 115, 120);
        ofColor c = topColor.getLerped(bottomColor, progress);

        backWall.addVertex(ofVec3f(housingLeft + 10, y, 0));
        backWall.addColor(c);
        backWall.addVertex(ofVec3f(housingRight - 10, y, 0));
        backWall.addColor(c);
    }
    backWall.draw();

    ofSetColor(100, 105, 110, 255);
    float thickness = 8;

    ofDrawRectangle(housingLeft, housingTop, thickness,
                    housingBottom - housingTop);
    ofDrawRectangle(housingRight - thickness, housingTop, thickness,
                    housingBottom - housingTop);
    ofDrawRectangle(housingLeft, housingTop, housingRight - housingLeft,
                    thickness);
    ofDrawRectangle(housingLeft, housingBottom - thickness,
                    housingRight - housingLeft, thickness);

    ofSetColor(140, 145, 150, 180);
    ofSetLineWidth(2);
    ofDrawLine(housingLeft + thickness, housingTop, housingLeft + thickness,
               housingBottom);
    ofDrawLine(housingRight - thickness, housingTop, housingRight - thickness,
               housingBottom);
}

void ofApp::drawFryerBasket() {
    ofColor wireColor(130, 135, 140, 200);
    float meshSpacing = 15;

    // Basket frame
    ofSetColor(wireColor);
    ofSetLineWidth(3);
    ofNoFill();
    ofDrawRectangle(basketLeftX, basketTopY, basketRightX - basketLeftX,
                    basketBottomY - basketTopY);

    // Horizontal wires
    ofSetLineWidth(1.5f);
    ofSetColor(wireColor);
    for (float y = basketTopY + meshSpacing; y < basketBottomY;
         y += meshSpacing) {
        ofDrawLine(basketLeftX, y, basketRightX, y);
    }

    // Diagonal cross wires
    ofSetLineWidth(1.0f);
    ofSetColor(wireColor.r, wireColor.g, wireColor.b, 140);
    int numCrosses = 4;
    float sectionWidth = (basketRightX - basketLeftX) / numCrosses;
    for (int i = 0; i < numCrosses; i++) {
        float x1 = basketLeftX + i * sectionWidth;
        float x2 = x1 + sectionWidth;
        ofDrawLine(x1, basketTopY, x2, basketBottomY);
    }

    ofFill();

    // Handle
    float handleAttachX = basketRightX;
    float handleAttachY = basketTopY + 15;
    float housingTop = fryerTopY - 15;
    float cornerY = housingTop - 12;
    float handleEndX = fryerRightX + 180;
    float gripLength = 60;

    ofSetColor(wireColor);
    ofSetLineWidth(4);
    ofDrawLine(handleAttachX, handleAttachY, handleAttachX, cornerY);
    ofDrawLine(handleAttachX, cornerY, handleEndX - gripLength, cornerY);

    ofSetColor(30, 30, 35, 255);
    ofDrawRectRounded(handleEndX - gripLength, cornerY - 7, gripLength, 14, 3);

    ofSetColor(50, 50, 55, 230);
    ofSetLineWidth(1.5f);
    for (int i = 0; i < 5; i++) {
        float rx = handleEndX - gripLength + 10 + i * 11;
        ofDrawLine(rx, cornerY - 4, rx, cornerY + 4);
    }
}

void ofApp::drawControlPanel() {
    float displayWidth = 110;
    float displayHeight = 32;
    float displayX = ((fryerLeftX + fryerRightX) / 2) - (displayWidth / 2);
    float displayY = oilBottomY + 18;

    // Background
    ofSetColor(15, 20, 25, 255);
    ofDrawRectRounded(displayX, displayY, displayWidth, displayHeight, 3);

    // Border
    ofSetColor(60, 65, 70, 200);
    ofSetLineWidth(1);
    ofNoFill();
    ofDrawRectRounded(displayX, displayY, displayWidth, displayHeight, 3);
    ofFill();

    // Temperature display
    float tempNorm = ofMap(oilTemperature, 160, 190, 0, 1, true);
    ofColor displayColor =
        ofColor(255, 120, 50).getLerped(ofColor(255, 50, 30), tempNorm);
    ofSetColor(displayColor);
    ofDrawBitmapString(ofToString((int)oilTemperature) + " C", displayX + 30,
                       displayY + 20);
}
