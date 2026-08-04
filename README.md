# BloomEngine
Custom C++ Game Engine with Bloom, Editor Tools, and Real-Time Rendering.

# BloomEngine
Custom C++ Game Engine focused on real-time rendering, bloom effects, editor tools, and a modular architecture.

BloomEngine is built from scratch using modern C++ and a clean component-based design.  
It includes a custom renderer, scene system, editor panels, math utilities, platform layer, and full engine core.

---

## 🚀 Features

### 🔹 Rendering
- Real-Time Rendering Pipeline  
- Bloom Post-Processing  
- HDR Support  
- Custom Material System  
- Mesh & Texture Loading  
- Camera System (FPS / Editor)  
- Frustum Culling  
- GPU Abstractions (Renderer Module)

### 🔹 Engine Core
- Entity Component System (ECS)  
- Scene Graph  
- Resource Manager  
- Event System  
- Time & DeltaTime  
- Logging System  

### 🔹 Editor Tools
- Custom Editor Panels  
- Scene Hierarchy  
- Inspector  
- Real-Time Property Editing  
- Gizmos (Translate / Rotate / Scale)  
- Viewport Rendering  

### 🔹 Platform Layer
- Window Creation  
- Input Handling  
- File System Utilities  

### 🔹 Math Module
- Vectors, Matrices, Quaternions  
- Transform System  
- Collision Helpers  

---

## 📁 Project Structure




BloomEngine/
│
├── Core/          # Engine core systems
├── Engine/        # ECS, scenes, resources
├── Editor/        # Editor UI and tools
├── Renderer/      # Rendering pipeline
├── External/      # External libs (except ImGui)
├── Math/          # Math utilities
├── Platform/      # Window, input, OS layer
├── Scene/         # Scene management
├── Utils/         # Helpers and utilities
│
├── CMakeLists.txt
├── main.cpp
├── vcpkg.json
└── .gitignore




---

## 📦 Dependencies

BloomEngine uses the following external libraries:

- **ImGui** (UI)  
  https://github.com/ocornut/imgui  

- **GLM** (Math)  
- **STB Image** (Textures)  
- **Assimp** (Model Loading)  
- **VCPKG** for dependency management

---

## 🛠 Build Instructions

### 1️⃣ Install VCPKG


git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh



### 2️⃣ Install dependencies

vcpkg install imgui glm assimp stb


### 3️⃣ Configure the project
cmake -B build -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake


### 4️⃣ Build

cmake --build build


---

## 🎮 Running the Engine

After building, run the executable generated in:

build/bin/


The editor will open with:

- Scene viewport  
- Hierarchy panel  
- Inspector  
- Real-time rendering  
- Bloom enabled  

---

## 📂 Assets

Assets are not included in the repository due to size limits.

Download them here:

**Assets Download:**  
👉 *[Add your Google Drive link here]*

Place the assets inside:

BloomEngine/Assets/


---

## 📸 Screenshots / Demo

(Add images or a YouTube link when you have them)

---

## 👤 Author

**Kevin Pedrero Pastor**  
Indie Engine Developer  
C++ / Rendering / Tools / Engine Architecture

---

## ⭐ Contribute

Pull requests are welcome.  
If you want to improve rendering, editor tools, or engine architecture, feel free to collaborate.










