# Deep Frying Simulation

A physics-based simulation of deep frying potatoes built with openFrameworks. This project models the thermodynamic and
physical behavior of potato fries during the cooking process, including heat transfer, moisture evaporation, density
changes, crust formation, and buoyancy dynamics.

## Features

- Real-time physics simulation of potato frying
- Temperature-dependent oil properties (density and viscosity)
- Moisture evaporation and crust formation
- Bubble generation and particle systems
- Interactive drag-and-drop fry placement
- Visual feedback showing cooking progression

## Physical Models

### Potato Physics

- **Heat transfer**: Newton's Law of Cooling with phase-dependent coefficients
- **Density**: Linear interpolation from raw (1.08 g/cm³) to fried (0.60 g/cm³)
- **Buoyancy**: Archimedes' principle with viscous drag
- **Cookedness**: Maillard reaction kinetics

### Oil Thermodynamics

- **Density**: Linear thermal expansion ρ(T) = ρ₀ - α(T - T₀)
- **Viscosity**: Arrhenius temperature dependence μ = A \* exp(Ea/RT)

## Web Demo

[Live Demo](https://erichobson.com/deep-frying-simulation)

## Building from Source

### Prerequisites

- openFrameworks
- C++ compiler
- For web build: Emscripten

### Compilation

```bash
make
```

### Web Build

```bash
emmake make
```

## Project Structure

```
src/
├── ofApp.cpp/h      - Main application loop and rendering
├── Potato.cpp/h     - Potato physics and thermodynamics
├── Oil.cpp/h        - Oil surface simulation
├── Bubble.cpp/h     - Bubble particle system
└── main.cpp         - Entry point
```

## Controls

- **Mouse drag**: Pick up and move fries
- **Click**: Drop fries into oil
- **Arrow keys**: Adjust oil temperature
