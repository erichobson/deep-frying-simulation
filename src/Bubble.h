#pragma once

#include "ofMain.h"

/**
 * Simulates steam bubbles generated during potato frying. Bubble behavior
 * is classified into three types based on formation depth ratio (h/R) [5]:
 *
 *   Type 0 (Explosion):   h/R < 0.5  - Rapid fragmentation near surface
 *   Type 1 (Elongated):   h/R < 1.5  - Stretched shape, fast rise
 *   Type 2 (Oscillating): h/R >= 1.5 - Large wobbling bubbles
 *
 * Reference:
 *   [5] Kiyama, A., et al. (2022). "Morphology of bubble dynamics and sound
 *       in heated oil." Physics of Fluids, 34(6).
 */
class Bubble {
   public:
    Bubble(ofVec2f pos, float oilTemp, float depthBelowSurface,
           float oilSurfaceY);

    void update(float dt, float oilViscosity);
    void draw();
    void applyForce(ofVec2f force);

    ofVec2f position;
    ofVec2f velocity;
    ofVec2f acceleration;

    float startSize;
    float endSize;
    float size;
    float lifespan;
    float life;
    float oscillation;
    float oscillationSpeed;
    float wobblePhase;
    float oilSurfaceY;
    float initialDepth;

    int bubbleType;
    bool isDead;
    bool reachedSurface;

    ofColor color;

    std::vector<ofVec2f> trail;
    int maxTrailLength;
};
