# DroneSimulator 🚁

A lightweight 3D simulation of a drone in a real-time rendered environment.  
The project includes custom models, shaders, and textures, and demonstrates a simple but effective graphics pipeline in C++.


## 🔍 Overview

DroneSimulator renders a drone model flying through a 3D environment with real-time controls.  
The rendering pipeline supports custom shaders for lighting, texturing, and visual effects.  

Key goals:
- Practice real-time rendering techniques  
- Manage 3D assets (models, textures) efficiently  
- Explore custom shader programming  

---

## ✨ Features

- Real-time rendering of drone and environment objects  
- Support for **custom shaders** (vertex + fragment)  
- Basic **camera control** (free camera and drone follow)  
- **Model loading** with textures  
- Lightweight build with **CMake** and portable code  

---

## 🧰 Tech Stack

- **C++** – core application logic  
- **GLSL / ShaderLab** – shaders (lighting, materials, textures)  
- **CMake** – build configuration  
- **Python** – build helper script (`compile_and_run.py`)  
- Assets:
  - `models/` – 3D drone + environment  
  - `textures/` – surfaces, sky, materials  
  - `shaders/` – vertex & fragment shaders  


## ⚙️ Getting Started

### Build & Run

```bash
# Clone the repo
git clone https://github.com/peppetort/dronesimulator.git
cd dronesimulator


# Compile and run with the helper script
python3 compile_and_run.py
````

Or build manually:

```bash
mkdir build && cd build
cmake ..
make
./DroneSimulator
```

---

## 📂 Project Structure

```
DroneSimulator/
├── shaders/               # GLSL shaders
├── models/                # 3D models (drone, environment)
├── textures/              # Textures and materials
├── DroneSimulator.cpp     # Main entry point
├── DroneSimulator.hpp
├── Models.hpp             # Model loading utilities
├── compile_and_run.py     # Build and run helper
└── CMakeLists.txt         # Build configuration
```

## ⚡ Technical Details

* **Shaders**: Vertex + Fragment shaders for lighting, surface shading, texture mapping
* **Model Loader**: Loads 3D meshes and applies textures
* **Rendering**: Optimized for steady FPS on common GPUs
* **Flight Logic**:
  * Simplified physics model for thrust and inertia
  * Smooth acceleration/deceleration for realistic feel
  * Object tilting and inclination directly tied to motor power values
* **Controls**: Camera movement and basic drone view
