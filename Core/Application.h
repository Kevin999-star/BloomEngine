#pragma once
#include <sapi.h>
#include <sphelper.h>

#include "../Platform/Window.h"
#include "Camera.h"
#include "../Engine/Graphics/DX12/Core/RendererDX12.h"
#include "../Renderer/Shader.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Texture.h"
#include "../Renderer/VertexArray.h"
#include "../Renderer/VertexBuffer.h"
#include "../Renderer/IndexBuffer.h"
#include <glm/glm.hpp>
#include <memory>
#include "../Scene/Scene.h"

Mesh GenerateSimpleTextMesh(const std::string& text, float depth);

enum class RenderBackend { OpenGL, DirectX12 };

class Application
{
public:
    Application(RenderBackend backend);
    void Run();
    void Update(float dt);
    void Render();
    ~Application();
    Scene scene;
    void ProcessVoiceCommand(const std::wstring& cmd);
   
    


private:
    
    
    Window* window;
    Camera* camera;
    RendererDX12* rendererDX12 = nullptr;
    
    RenderBackend backend;
    Material* cubeMat;
    Material* defaultMat;
    ISpRecognizer* recognizer;
    ISpRecoContext* context;
    ISpRecoGrammar* grammar;
    std::wstring lastVoiceCommand = L"";
    HANDLE hEvent;

    
  // OpenGL resources
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;

    unsigned int finalFBO = 0;
    unsigned int finalColor = 0;
    unsigned int finalDepth = 0;

    unsigned int noiseTextureID = 0;

    unsigned int bloomFBO = 0;
    unsigned int bloomColor = 0;

    unsigned int pingpongFBO[2] = { 0, 0 };
    unsigned int pingpongColor[2] = { 0, 0 };
    
    // Suelo simple
    std::unique_ptr<Shader> simpleShader;
    unsigned int simplePlaneVAO = 0;
    unsigned int simplePlaneVBO = 0;
    unsigned int simplePlaneIBO = 0;
    unsigned int skyVAO;
    unsigned int skyVBO;
    unsigned int skyboxTex;

    int selectedEntity = -1;
    std::unique_ptr<Shader> gridShader;
    std::vector<int> selectedEntities;

    // Buffers del cubo
    VertexArray* cubeVAO = nullptr;
    VertexBuffer* cubeVBO = nullptr;
    IndexBuffer* cubeIBO = nullptr;
   
    
    // Buffers del plano
    VertexArray* planeVAO = nullptr;
    VertexBuffer* planeVBO = nullptr;
    IndexBuffer* planeIBO = nullptr;
   
    //Quad:
    Mesh quadMesh;
    VertexArray* quadVAO2;
    VertexBuffer* quadVBO2;
    IndexBuffer* quadIBO2;

    //Triangle:
    Mesh triangleMesh;
    VertexArray* triangleVAO;
    VertexBuffer* triangleVBO;
    IndexBuffer* triangleIBO;

    //Circle(32 Segments):
    Mesh circleMesh;
    VertexArray* circleVAO;
    VertexBuffer* circleVBO;
    IndexBuffer* circleIBO;

    //Pyramid:
    Mesh pyramidMesh;
    VertexArray* pyramidVAO;
    VertexBuffer* pyramidVBO;
    IndexBuffer* pyramidIBO;

    //SPHERE:
    Mesh sphereMesh;
    VertexArray* sphereVAO;
    VertexBuffer* sphereVBO;
    IndexBuffer* sphereIBO;

    //CYLINDER:
    Mesh cylinderMesh;
    VertexArray* cylinderVAO;
    VertexBuffer* cylinderVBO;
    IndexBuffer* cylinderIBO;

    //Cone:
    Mesh coneMesh;
    VertexArray* coneVAO;
    VertexBuffer* coneVBO;
    IndexBuffer* coneIBO;

    //Torus:
    Mesh torusMesh;
    VertexArray* torusVAO;
    VertexBuffer* torusVBO;
    IndexBuffer* torusIBO;

    //Table:
    VertexArray* tableVAO;
    VertexBuffer* tableVBO;
    IndexBuffer* tableIBO;

    Mesh tableMesh;

  //Knife:
    VertexArray* knifeVAO;
    VertexBuffer* knifeVBO;
    IndexBuffer* knifeIBO;

    Mesh knifeMesh;



   



// 🔥 SHADERS (FALTABAN) 🔥
    Shader lightingShader;
    Shader lineShader;
    Shader shadowShader;
    Shader skyShader;
    Shader fogShader;
    Shader thresholdShader;
    Shader blurShader;
    Shader combineShader;
    Shader floorGridShader;
    Shader floorLightingShader;


    // Meshes
    Mesh cubeMesh;
    Mesh planeMesh;
   
    // Tube (cilindro hueco)
    Mesh tubeMesh;
    VertexArray* tubeVAO;
    VertexBuffer* tubeVBO;
    IndexBuffer* tubeIBO;


    // Capsule
    Mesh capsuleMesh;
    VertexArray* capsuleVAO;
    VertexBuffer* capsuleVBO;
    IndexBuffer* capsuleIBO;

   

    // Door
    Mesh doorMesh;
    VertexArray* doorVAO;
    VertexBuffer* doorVBO;
    IndexBuffer* doorIBO;

  
    //DoorFrame:
    Mesh doorFrameMesh;
    VertexArray* doorFrameVAO;
    VertexBuffer* doorFrameVBO;
    IndexBuffer* doorFrameIBO;

    // Realistic Ball
    Mesh ballMesh;
    VertexArray* ballVAO;
    VertexBuffer* ballVBO;
    IndexBuffer* ballIBO;

    //Ladder:
    Mesh ladderMesh;
    VertexArray* ladderVAO;
    VertexBuffer* ladderVBO;
    IndexBuffer* ladderIBO;

   //Stair:
    Mesh modernStairMesh;
    VertexArray* modernStairVAO;
    VertexBuffer* modernStairVBO;
    IndexBuffer* modernStairIBO;

   

    // Textures
    Texture* diffuse;
    Texture* normalMap;

    Mesh droneMesh;




    // Scene state
    glm::vec3 cubePosition;
    glm::vec3 lightPos;
    glm::vec3 floorPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cubeRotation;
    glm::vec3 cubeScale = glm::vec3(1.0f);
    glm::vec3 floorScale = glm::vec3(1.0f);
    glm::vec3 floorRotation;
    glm::vec3 selectedColor = glm::vec3(1.0f, 1.0f, 1.0f);

   
};
