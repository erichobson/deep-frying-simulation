/**
 * Deep-Frying Simulation
 *
 * A physics-based simulation of deep frying a potato fry in hot oil.
 * Built with openFrameworks.
 *
 * Features:
 *   - Realistic oil thermodynamics (Arrhenius viscosity, thermal expansion)
 *   - Physics-based potato behavior (heat transfer, moisture loss, buoyancy)
 *   - Visual effects (bubbles, steam, oil surface effects, heat haze)
 *   - Interactive controls (temperature adjustment, drag fry, reset)
 *
 * Controls:
 *   UP/DOWN  - Adjust oil temperature (160-190°C)
 *   SPACE    - Drop/remove potato fry
 *   P        - Pause/unpause simulation
 *   R        - Reset simulation
 *   MOUSE    - Drag fry in oil
 *
 * Eric Hobson
 * COMP 4900L - Fall 2025
 */

#include "ofApp.h"
#include "ofMain.h"

int main() {
    ofSetupOpenGL(1024, 768, OF_WINDOW);
    ofRunApp(new ofApp());
}
