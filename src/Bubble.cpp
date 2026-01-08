#include "Bubble.h"

Bubble::Bubble(ofVec2f pos, float oilTemp, float depthBelowSurface,
               float surfaceY) {
    position = pos;
    oilSurfaceY = surfaceY;
    initialDepth = depthBelowSurface;
    reachedSurface = false;

    // Bubble type classification based on depth-to-radius ratio (h/R) [5]
    float estimatedRadius = ofRandom(2.5f, 7.0f);
    float h_R_ratio = depthBelowSurface / estimatedRadius;

    if (h_R_ratio < 0.5f) {
        bubbleType = 0;  // Explosion
    } else if (h_R_ratio < 1.5f) {
        bubbleType = 1;  // Elongated
    } else {
        bubbleType = 2;  // Oscillating
    }

    // Type-specific initialization (oscillationSpeed set only for Type 2)
    if (bubbleType == 0) {
        velocity = ofVec2f(ofRandom(-70, 70), ofRandom(-140, -200));
        startSize = ofRandom(3, 7);
        endSize = startSize * ofRandom(0.15f, 0.35f);
        lifespan = ofRandom(0.4f, 0.9f);
        maxTrailLength = 3;
    } else if (bubbleType == 1) {
        velocity = ofVec2f(ofRandom(-20, 20), ofRandom(-150, -220));
        startSize = ofRandom(3, 6);
        endSize = startSize * ofRandom(1.8f, 2.8f);
        lifespan = ofRandom(0.7f, 1.4f);
        maxTrailLength = 6;
    } else {
        velocity = ofVec2f(ofRandom(-25, 25), ofRandom(-80, -130));
        startSize = ofRandom(6, 14);
        endSize = startSize * ofRandom(0.9f, 1.3f);
        lifespan = ofRandom(1.2f, 2.5f);
        oscillationSpeed = ofRandom(14, 30);
        maxTrailLength = 8;
    }

    // Common initialization
    size = startSize;
    life = 1.0f;
    oscillation = 0.0f;
    if (bubbleType != 2) oscillationSpeed = 0.0f;
    wobblePhase = ofRandom(0, TWO_PI);
    acceleration = ofVec2f(0, 0);
    isDead = false;

    // Temperature-dependent appearance
    int baseIntensity = ofMap(oilTemp, 160, 190, 200, 255, true);
    color = ofColor(baseIntensity, baseIntensity - 5, baseIntensity - 30, 200);
}

void Bubble::update(float dt, float oilViscosity) {
    life -= dt / lifespan;
    if (life <= 0) {
        isDead = true;
        return;
    }

    // Surface detection
    if (position.y <= oilSurfaceY + 5 && !reachedSurface) {
        reachedSurface = true;
        life = std::min(life, 0.15f);
    }

    // Viscous drag: F = -μ * c * v
    float dragCoeff = 20.0f;
    ofVec2f drag = velocity * -1.0f * oilViscosity * dragCoeff;
    applyForce(drag);

    // Horizontal wobble
    float wobble = sin(wobblePhase + ofGetElapsedTimef() * 8) * 15;
    acceleration.x += wobble * dt;

    // Integration
    velocity += acceleration * dt;
    position += velocity * dt;

    // Size interpolation with quadratic easing
    float lifeRatio = 1.0f - life;
    float easedRatio = lifeRatio * lifeRatio;
    size = ofLerp(startSize, endSize, easedRatio);

    // Oscillating bubble size variation
    if (bubbleType == 2) {
        oscillation += oscillationSpeed * dt;
        float oscillationAmount = sin(oscillation) * (startSize * 0.22f);
        size += oscillationAmount;
    }

    // Trail update
    if (trail.size() == 0 || position.distance(trail.back()) > 3) {
        trail.push_back(position);
        if (trail.size() > maxTrailLength) {
            trail.erase(trail.begin());
        }
    }

    // Alpha fade
    float alphaBase = 200;
    if (reachedSurface) {
        float popProgress = 1.0f - (life / 0.15f);
        color.a = ofMap(popProgress, 0, 1, alphaBase, 0);
        size *= (1.0f + popProgress * 0.5f);
    } else {
        color.a = ofMap(life, 0, 1.0f, 60, alphaBase, true);
    }

    acceleration *= 0;
}

void Bubble::draw() {
    if (isDead) return;

    // Trail rendering
    if (trail.size() > 1 && !reachedSurface) {
        for (size_t i = 0; i < trail.size() - 1; i++) {
            float t = (float)i / (trail.size() - 1);
            float trailAlpha = ofMap(t, 0, 1, 5, 35) * (color.a / 200.0f);
            float trailSize = size * ofMap(t, 0, 1, 0.15f, 0.5f);

            ofSetColor(color.r + 20, color.g + 20, color.b + 10, trailAlpha);
            ofDrawCircle(trail[i], trailSize);
        }
    }

    if (bubbleType == 1 && !reachedSurface) {
        // Elongated bubble
        ofPushMatrix();
        ofTranslate(position);
        float angle = atan2(velocity.y, velocity.x) + HALF_PI;
        ofRotateRad(angle);

        float speed = velocity.length();
        float stretchFactor = ofMap(speed, 50, 250, 1.8f, 3.2f, true);
        float squishFactor = ofMap(speed, 50, 250, 0.45f, 0.28f, true);

        // Membrane
        ofSetColor(color.r - 10, color.g - 10, color.b - 15, color.a * 0.3f);
        ofDrawEllipse(0, 0, size * squishFactor * 1.15f,
                      size * stretchFactor * 1.08f);

        // Body
        ofSetColor(color);
        ofDrawEllipse(0, 0, size * squishFactor, size * stretchFactor);

        // Interior
        ofSetColor(color.r + 40, color.g + 35, color.b + 25, color.a * 0.35f);
        ofDrawEllipse(size * 0.02f, -size * 0.15f, size * squishFactor * 0.7f,
                      size * stretchFactor * 0.65f);

        // Highlights
        ofSetColor(255, 252, 240, color.a * 0.75f);
        ofDrawEllipse(-size * 0.08f, -size * stretchFactor * 0.35f,
                      size * 0.12f, size * 0.5f);

        ofSetColor(255, 248, 230, color.a * 0.4f);
        ofDrawEllipse(size * 0.06f, size * stretchFactor * 0.25f, size * 0.08f,
                      size * 0.25f);

        ofPopMatrix();

    } else if (reachedSurface) {
        // Popping bubble
        float popProgress = 1.0f - (life / 0.15f);
        float ringSize = size * (1.0f + popProgress * 2.5f);

        // Expanding rings
        for (int ring = 0; ring < 3; ring++) {
            float ringOffset = ring * 0.15f;
            float thisRingProgress = ofClamp(popProgress - ringOffset, 0, 1);
            if (thisRingProgress <= 0) continue;

            float thisRingSize = ringSize * (0.6f + ring * 0.25f) *
                                 (1.0f + thisRingProgress * 0.5f);
            float ringAlpha =
                color.a * (1.0f - thisRingProgress) * (1.0f - ring * 0.3f);

            ofNoFill();
            ofSetLineWidth((2.5f - ring * 0.6f) * (1.0f - thisRingProgress));
            ofSetColor(color.r + 30, color.g + 25, color.b + 15, ringAlpha);
            ofDrawCircle(position, thisRingSize);
        }
        ofFill();

        // Scattered droplets
        int numParticles = 6;
        for (int i = 0; i < numParticles; i++) {
            float particleAngle =
                (TWO_PI / numParticles) * i + popProgress * PI * 0.5f;
            float particleDist = ringSize * (0.5f + popProgress * 0.4f);
            float px = position.x + cos(particleAngle) * particleDist;
            float py = position.y + sin(particleAngle) * particleDist * 0.4f -
                       popProgress * popProgress * 8;

            float dropletSize = size * 0.12f * (1.0f - popProgress * 0.7f);
            float dropletAlpha = color.a * 0.6f * (1.0f - popProgress);

            ofSetColor(color.r + 20, color.g + 15, color.b + 5, dropletAlpha);
            ofDrawCircle(px, py, dropletSize);
            ofSetColor(255, 250, 240, dropletAlpha * 0.5f);
            ofDrawCircle(px - dropletSize * 0.3f, py - dropletSize * 0.3f,
                         dropletSize * 0.35f);
        }

    } else {
        // Standard bubble (explosion and oscillating types)
        float wobbleAmount = sin(wobblePhase + ofGetElapsedTimef() * 6) * 0.08f;
        float scaleX = 1.0f + wobbleAmount;
        float scaleY = 1.0f - wobbleAmount;

        ofPushMatrix();
        ofTranslate(position);

        // Outer glow
        for (int i = 3; i >= 0; i--) {
            float glowSize = size * (1.2f + i * 0.15f);
            float glowAlpha = color.a * 0.06f * (4 - i) / 4.0f;
            ofSetColor(color.r + 30, color.g + 25, color.b + 15, glowAlpha);
            ofDrawEllipse(0, 0, glowSize * scaleX, glowSize * scaleY);
        }

        // Membrane
        ofSetColor(color.r - 15, color.g - 10, color.b, color.a * 0.4f);
        ofDrawEllipse(0, 0, size * 1.08f * scaleX, size * 1.08f * scaleY);

        // Body
        ofSetColor(color);
        ofDrawEllipse(0, 0, size * scaleX, size * scaleY);

        // Interior gradient
        ofSetColor(color.r + 45, color.g + 40, color.b + 30, color.a * 0.45f);
        ofDrawEllipse(size * 0.08f, size * 0.05f, size * 0.65f * scaleX,
                      size * 0.6f * scaleY);

        ofSetColor(color.r + 60, color.g + 55, color.b + 40, color.a * 0.25f);
        ofDrawEllipse(size * 0.1f, size * 0.08f, size * 0.4f * scaleX,
                      size * 0.35f * scaleY);

        // Highlights
        float highlightIntensity = 0.85f;
        ofSetColor(255, 253, 245, color.a * highlightIntensity);
        ofDrawEllipse(-size * 0.32f * scaleX, -size * 0.32f * scaleY,
                      size * 0.28f, size * 0.22f);

        ofSetColor(255, 255, 252, color.a * 0.9f);
        ofDrawCircle(-size * 0.28f * scaleX, -size * 0.38f * scaleY,
                     size * 0.1f);

        ofSetColor(255, 250, 235, color.a * 0.35f);
        ofDrawEllipse(-size * 0.1f * scaleX, -size * 0.52f * scaleY,
                      size * 0.18f, size * 0.1f);

        // Rim light arc
        ofSetColor(255, 245, 220, color.a * 0.3f);
        ofNoFill();
        ofSetLineWidth(size * 0.08f);
        ofBeginShape();
        int arcSegments = 12;
        for (int seg = 0; seg <= arcSegments; seg++) {
            float arcAngle = ofMap(seg, 0, arcSegments, 30, 120) * DEG_TO_RAD;
            float ax = cos(arcAngle) * size * 0.85f * scaleX;
            float ay = sin(arcAngle) * size * 0.85f * scaleY;
            ofVertex(ax, ay);
        }
        ofEndShape();
        ofFill();

        // Bottom caustic
        ofSetColor(255, 248, 210, color.a * 0.2f);
        ofDrawEllipse(size * 0.15f * scaleX, size * 0.4f * scaleY, size * 0.2f,
                      size * 0.12f);

        ofPopMatrix();
    }
}

void Bubble::applyForce(ofVec2f force) { acceleration += force; }
