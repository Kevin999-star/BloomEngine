#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <stb_image.h>
#include <random>
#include <vector>
#include <cmath>

#include "Application.h"
#include "Time.h"
#include "Camera.h"
#include "../Platform/Input.h"
#include "../Renderer/Shader.h"
#include "../Renderer/VertexArray.h"
#include "../Renderer/VertexBuffer.h"
#include "../Renderer/IndexBuffer.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Texture.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
// === IMGUI HEADERS ===
#include "../External/ImGui/imgui.h"
#include "../External/ImGui/imgui_impl_glfw.h"
#include "../External/ImGui/imgui_impl_opengl3.h"
#include "../Editor/Gizmos.h"
#include "../External/ImGui/ImGuizmo.h"
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Application.h"
#include "Time.h"
#include "Camera.h"
#include "../Platform/Input.h"
#include <glm/gtc/type_ptr.hpp>   // <-- AÑADIR ESTO
#include <sapi.h>
#include <sphelper.h>
#include <glm/glm.hpp>
#include "Scene.h"
#include "../External/tinyobjloader/tiny_obj_loader.h"




Shader skyboxShader;
unsigned int skyboxVAO;
unsigned int skyboxVBO;
unsigned int skyCubemap;
bool isPlaying = false;
bool SetMouseLocked = false;
Camera* editorCamera;
Camera* playerCamera;
Camera* activeCamera;




// -----------------------------------------------------------------------------
// Helpers ruido FBM + Worley
// -----------------------------------------------------------------------------
float rand01()
{
    return rand() / (float)RAND_MAX;
}

float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

float smoothstep(float t)
{
    return t * t * (3 - 2 * t);
}

float perlin2D(float x, float y)
{
    int xi = (int)floor(x) & 255;
    int yi = (int)floor(y) & 255;

    float xf = x - floor(x);
    float yf = y - floor(y);

    float u = smoothstep(xf);
    float v = smoothstep(yf);

    float n00 = rand01();
    float n01 = rand01();
    float n10 = rand01();
    float n11 = rand01();

    float x1 = lerp(n00, n10, u);
    float x2 = lerp(n01, n11, u);

    return lerp(x1, x2, v);
}

float worley2D(float x, float y)
{
    int xi = (int)floor(x);
    int yi = (int)floor(y);

    float minDist = 9999.0f;

    for (int yy = -1; yy <= 1; yy++)
    {
        for (int xx = -1; xx <= 1; xx++)
        {
            float px = xi + xx + rand01();
            float py = yi + yy + rand01();

            float dx = px - x;
            float dy = py - y;

            float dist = sqrt(dx * dx + dy * dy);
            if (dist < minDist)
                minDist = dist;
        }
    }

    return minDist;
}

float fbm(float x, float y)
{
    float value = 0.0f;
    float amplitude = 0.5f;

    for (int i = 0; i < 5; i++)
    {
        value += perlin2D(x, y) * amplitude;
        x *= 2.0f;
        y *= 2.0f;
        amplitude *= 0.5f;
    }

    return value;
}


// -----------------------------------------------------------------------------
// Cubemap helper
// -----------------------------------------------------------------------------
unsigned int LoadCubemap(const std::vector<std::string>& faces)
{
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int w, h, channels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &channels, STBI_rgb_alpha);
        if (!data)
        {
            std::cout << "Error cargando: " << faces[i] << std::endl;
            continue;
        }

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
        );

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}

// -----------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------
Application::Application(RenderBackend backend)
    : backend(backend)
 {
    // 1. Ventana
    window = new Window(1080, 720, "My Engine");
    HWND hwnd = glfwGetWin32Window(window->GetNativeWindow());

    recognizer = nullptr;
    context = nullptr;
    grammar = nullptr;


    CoInitialize(NULL);
    CoCreateInstance(CLSID_SpInprocRecognizer, NULL, CLSCTX_INPROC_SERVER,
                     IID_ISpRecognizer, (void**)&recognizer);
   

    recognizer->SetInput(NULL, TRUE);
    recognizer->CreateRecoContext(&context);

    // Activar notificación por evento Win32
    context->SetNotifyWin32Event();
    hEvent = context->GetNotifyEventHandle();
    

// Activar solo eventos de reconocimiento
    context->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

    // Obtener handle del evento
  
    context->CreateGrammar(1, &grammar);
    grammar->LoadDictation(NULL, SPLO_STATIC);
    grammar->SetDictationState(SPRS_ACTIVE);

    // Obtener el handle del evento
    HANDLE hEvent = context->GetNotifyEventHandle();

    // 2. DirectX12
    if (backend == RenderBackend::DirectX12)
    {
        rendererDX12 = new RendererDX12();
        rendererDX12->Initialize(hwnd, 1280, 720);

        Input::Init(window->GetNativeWindow());
        camera = new Camera(
            glm::vec3(0.0f, 0.0f, 3.0f),
            -90.0f,
            0.0f,
            45.0f,
            1280.0f / 720.0f
        );
        // Cámara del jugador
        playerCamera = new Camera(
            glm::vec3(0.0f, 1.8f, 0.0f),
            -90.0f,
            0.0f,
            60.0f,
            1280.0f / 720.0f
        );

        // Cámara activa
        activeCamera = editorCamera;


        cubePosition = glm::vec3(0.0f, 1.0f, -5.0f);
        lightPos = glm::vec3(3.0f, 6.0f, 3.0f);
        return;
    }

    // 3. OpenGL
    if (backend == RenderBackend::OpenGL)
    {
        glfwMakeContextCurrent(window->GetNativeWindow());

        if (glfwJoystickPresent(GLFW_JOYSTICK_1))
        {
            std::cout << "Mando Xbox detectado" << std::endl;
        }
        else
        {
            std::cout << "No hay mando conectado" << std::endl;
        }




        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize GLAD");
        
      
        float simplePlaneVertices[] = {
            // posX posY posZ   u   v
            -20.0f, 0.0f, -20.0f, 0.0f, 0.0f,
             20.0f, 0.0f, -20.0f, 1.0f, 0.0f,
             20.0f, 0.0f,  20.0f, 1.0f, 1.0f,
            -20.0f, 0.0f,  20.0f, 0.0f, 1.0f
        };

        
        unsigned int simplePlaneIndices[] = {
            0, 1, 2,
            2, 3, 0
         };

         glGenVertexArrays(1, &simplePlaneVAO);
         glGenBuffers(1, &simplePlaneVBO);
         glGenBuffers(1, &simplePlaneIBO);

          glBindVertexArray(simplePlaneVAO);

          glBindBuffer(GL_ARRAY_BUFFER, simplePlaneVBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(simplePlaneVertices), simplePlaneVertices, GL_STATIC_DRAW);

          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, simplePlaneIBO);
          glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(simplePlaneIndices), simplePlaneIndices, GL_STATIC_DRAW);

          glEnableVertexAttribArray(0);
          glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

          glEnableVertexAttribArray(1);
          glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


          glBindVertexArray(0);


        lightingShader = Shader("Assets/Shaders/lighting.vert", "Assets/Shaders/lighting.frag");
        lineShader = Shader("Assets/Shaders/line.vert", "Assets/Shaders/line.frag");
        shadowShader = Shader("Assets/Shaders/shadow.vert", "Assets/Shaders/shadow.frag");
        skyShader = Shader("Assets/Shaders/sky_fullscreen.vert", "Assets/Shaders/sky_fullscreen.frag");
        skyboxShader = Shader("Assets/Shaders/skybox.vert", "Assets/Shaders/skybox.frag");
        fogShader = Shader("Assets/Shaders/fog.vert", "Assets/Shaders/fog.frag");
        thresholdShader = Shader("Assets/Shaders/fullscreen_quad.vert", "Assets/Shaders/bloom_threshold.frag");
        blurShader = Shader("Assets/Shaders/fullscreen_quad.vert", "Assets/Shaders/bloom_blur.frag");
        combineShader = Shader("Assets/Shaders/fullscreen_quad.vert", "Assets/Shaders/bloom_combine.frag");

        //Dron:
        static  Mesh droneMesh;
        droneMesh.LoadFromOBJ("Assets/Models/Drone/drone.obj");






       Texture* diffuse = new Texture("Assets/Textures/floortilescratched_diffuse.png");
       Texture* normalMap = new Texture("Assets/Textures/tiles_normal.png");


       std::vector<std::string> skyFaces =
       {
           "Assets/Textures/Skybox/right.png",
           "Assets/Textures/Skybox/left.png",
           "Assets/Textures/Skybox/top.png",
           "Assets/Textures/Skybox/bottom.png",
           "Assets/Textures/Skybox/front.png",
           "Assets/Textures/Skybox/back.png"
       };

       skyCubemap = LoadCubemap(skyFaces);
       std::cout << "Cubemap ID: " << skyCubemap << std::endl;


      

       float skyboxVertices[] = {
           // posiciones del cubo (solo posición)
           -1.0f,  1.0f, -1.0f,
           -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
           -1.0f,  1.0f, -1.0f,

           -1.0f, -1.0f,  1.0f,
           -1.0f, -1.0f, -1.0f,
           -1.0f,  1.0f, -1.0f,
           -1.0f,  1.0f, -1.0f,
           -1.0f,  1.0f,  1.0f,
           -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

           -1.0f, -1.0f,  1.0f,
           -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
           -1.0f, -1.0f,  1.0f,

           -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
           -1.0f,  1.0f,  1.0f,
           -1.0f,  1.0f, -1.0f,

           -1.0f, -1.0f, -1.0f,
           -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
           -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
       };

       
       glGenVertexArrays(1, &skyboxVAO);
       glGenBuffers(1, &skyboxVBO);
       glBindVertexArray(skyboxVAO);
       glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
       glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
       glEnableVertexAttribArray(0);
       glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



        // === MATERIAL POR DEFECTO ===



        // === SHADER SIMPLE ===
        simpleShader = std::make_unique<Shader>(
            "Assets/Shaders/simple.vert",
            "Assets/Shaders/simple.frag");

        gridShader = std::make_unique<Shader>(
            "Assets/Shaders/simple.vert",              // tu vertex shader real
            "Assets/Shaders/grid_fullscreen.frag"      // tu fragment shader real
        );


        floorGridShader = Shader("Assets/Shaders/simple.vert",
            "Assets/Shaders/floor_grid.frag");



        // Crear VAOs correctamente (punteros)
        cubeVAO = new VertexArray();
        planeVAO = new VertexArray();


        // ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window->GetNativeWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 330");

        // Estado GL
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.35f, 0.65f, 1.0f, 1.0f);

      

        // Input + cámara
        Input::Init(window->GetNativeWindow());
        camera = new Camera(
            glm::vec3(0.0f, 0.0f, 3.0f),
            -90.0f,
            0.0f,
            45.0f,
            1280.0f / 720.0f
        );

        playerCamera = new Camera(
            glm::vec3(0.0f, 1.8f, 0.0f),   // altura jugador
            -90.0f,
            0.0f,
            60.0f,
            1280.0f / 720.0f
        );

        activeCamera = editorCamera;



        // === RESET AUTOMÁTICO DE ESCENA ===
        floorPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        floorRotation = glm::vec3(0.0f);
        floorScale = glm::vec3(1.0f);

        cubePosition = glm::vec3(0.0f, 0.0f, 0.0f);
        cubeRotation = glm::vec3(0.0f);
        cubeScale = glm::vec3(1.0f);

        lightPos = glm::vec3(3.0f, 6.0f, 3.0f);


        // === QUAD FULLSCREEN ===
        float quadVertices[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };


        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        // === Geometría suelo + cubo ===
        float planeVertices[] = {
            -20.0f, 0.0f, -20.0f, 0,1,0, 0,0,  1,0,0,   0,0,1,
             20.0f, 0.0f, -20.0f, 0,1,0, 1,0,  1,0,0,   0,0,1,
             20.0f, 0.0f,  20.0f, 0,1,0, 1,1,  1,0,0,   0,0,1,
            -20.0f, 0.0f,  20.0f, 0,1,0, 0,1,  1,0,0,   0,0,1
        };

        uint32_t planeIndices[] = {
            0,1,2,
            2,3,0
        };

        float cubeVertices[] = {
            -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
             0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
             0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
            -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,

            -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,
             0.5f,-0.5f, 0.5f, 0,0,1, 1,0,
             0.5f, 0.5f, 0.5f, 0,0,1, 1,1,
            -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
        };

        float quadVertices2[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
      };

        //Quad:
        uint32_t quadIndices2[] = { 0,1,2, 2,3,0 };

        quadVAO2 = new VertexArray();
        quadVAO2->Bind();

        quadVBO2 = new VertexBuffer(quadVertices2, sizeof(quadVertices2));
        quadVBO2->Bind();

        quadIBO2 = new IndexBuffer(quadIndices2, 6);
        quadIBO2->Bind();

        quadVAO2->AddVertexBuffer(0, 3, 3 * sizeof(float), (void*)0);

        quadMesh.vao = quadVAO2->GetID();
        quadMesh.indexCount = 6;
        quadMesh.material = new Material();

        //Triangule:
        float triangleVertices[] = {
            // Vértice 0
            -0.5f, -0.5f, 0.0f,   0,0,1,   0,0,

            // Vértice 1
             0.5f, -0.5f, 0.0f,   0,0,1,   1,0,

             // Vértice 2
              0.0f,  0.5f, 0.0f,   0,0,1,   0.5f,1
        };


        uint32_t triangleIndices[] = { 0,1,2 };
        triangleVAO = new VertexArray();
        triangleVAO->Bind();

        triangleVBO = new VertexBuffer(triangleVertices, sizeof(triangleVertices));
        triangleVBO->Bind();

        triangleIBO = new IndexBuffer(triangleIndices, 3);
        triangleIBO->Bind();

        triangleVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                // posición
        triangleVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        triangleVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        triangleMesh.vao = triangleVAO->GetID();
        triangleMesh.indexCount = 3;
        triangleMesh.material = new Material();


        //CIRCLE:
        std::vector<float> circleVertices;
        std::vector<uint32_t> circleIndices;

        circleVertices.push_back(0.0f);
        circleVertices.push_back(0.0f);
        circleVertices.push_back(0.0f);

        int segments = 32;
        for (int i = 0; i <= segments; i++)
        {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            circleVertices.push_back(cos(angle));
            circleVertices.push_back(0.0f);
            circleVertices.push_back(sin(angle));
        }

        for (int i = 1; i <= segments; i++)
        {
            circleIndices.push_back(0);
            circleIndices.push_back(i);
            circleIndices.push_back(i + 1);
        }

        circleVAO = new VertexArray();
        circleVAO->Bind();

        circleVBO = new VertexBuffer(circleVertices.data(), circleVertices.size() * sizeof(float));
        circleIBO = new IndexBuffer(circleIndices.data(), circleIndices.size());

        circleVAO->AddVertexBuffer(0, 3, 3 * sizeof(float), (void*)0);

        circleMesh.vao = circleVAO->GetID();
        circleMesh.indexCount = circleIndices.size();
        circleMesh.material = new Material();

        //PYRAMID:
        float pyramidVertices[] = {
            // BASE (y = 0)
            -0.5f, 0.0f, -0.5f,   0, -1, 0,   0, 0,   // 0
             0.5f, 0.0f, -0.5f,   0, -1, 0,   1, 0,   // 1
             0.5f, 0.0f,  0.5f,   0, -1, 0,   1, 1,   // 2
            -0.5f, 0.0f,  0.5f,   0, -1, 0,   0, 1,   // 3

            // PUNTA
             0.0f, 1.0f,  0.0f,   0, 1, 0,    0.5f, 0.5f  // 4
        };

        uint32_t pyramidIndices[] = {
            // Base
            0, 1, 2,
            2, 3, 0,

            // Lados
            0, 1, 4,
            1, 2, 4,
            2, 3, 4,
            3, 0, 4
        };

        

        pyramidVAO = new VertexArray();
        pyramidVAO->Bind();

        pyramidVBO = new VertexBuffer(pyramidVertices, sizeof(pyramidVertices));
        pyramidIBO = new IndexBuffer(pyramidIndices, 18);

        pyramidVAO->AddVertexBuffer(0, 3, 3 * sizeof(float), (void*)0);

        pyramidMesh.vao = pyramidVAO->GetID();
        pyramidMesh.indexCount = 18;
        pyramidMesh.material = new Material();

        //SPHERE/SPHERE 32 * 16):
        std::vector<float> sphereVertices;
        std::vector<uint32_t> sphereIndices;

        int X_SEGMENTS = 32;
        int Y_SEGMENTS = 16;

        for (int y = 0; y <= Y_SEGMENTS; y++)
        {
            for (int x = 0; x <= X_SEGMENTS; x++)
            {
                float xSegment = (float)x / X_SEGMENTS;
                float ySegment = (float)y / Y_SEGMENTS;

                float xPos = cos(xSegment * 2.0f * 3.14159f) * sin(ySegment * 3.14159f);
                float yPos = cos(ySegment * 3.14159f);
                float zPos = sin(xSegment * 2.0f * 3.14159f) * sin(ySegment * 3.14159f);

                // POSICIÓN
                sphereVertices.push_back(xPos);
                sphereVertices.push_back(yPos);
                sphereVertices.push_back(zPos);

                // NORMAL (igual que posición normalizada)
                sphereVertices.push_back(xPos);
                sphereVertices.push_back(yPos);
                sphereVertices.push_back(zPos);

                // UV
                sphereVertices.push_back(xSegment);
                sphereVertices.push_back(ySegment);
            }
        }

        for (int y = 0; y < Y_SEGMENTS; y++)
        {
            for (int x = 0; x < X_SEGMENTS; x++)
            {
                int i0 = y * (X_SEGMENTS + 1) + x;
                int i1 = (y + 1) * (X_SEGMENTS + 1) + x;
                int i2 = (y + 1) * (X_SEGMENTS + 1) + (x + 1);
                int i3 = y * (X_SEGMENTS + 1) + (x + 1);

                sphereIndices.push_back(i0);
                sphereIndices.push_back(i1);
                sphereIndices.push_back(i2);

                sphereIndices.push_back(i0);
                sphereIndices.push_back(i2);
                sphereIndices.push_back(i3);
            }
        }

        sphereVAO = new VertexArray();
        sphereVAO->Bind();

        sphereVBO = new VertexBuffer(sphereVertices.data(), sphereVertices.size() * sizeof(float));
        sphereVBO->Bind();

        sphereIBO = new IndexBuffer(sphereIndices.data(), sphereIndices.size());
        sphereIBO->Bind();

        sphereVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        sphereVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        sphereVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        sphereMesh.vao = sphereVAO->GetID();
        sphereMesh.indexCount = sphereIndices.size();
        sphereMesh.material = new Material();

        //CYLINDER:
        std::vector<float> cylVertices;
        std::vector<uint32_t> cylIndices;

        float height = 1.0f;

        // --- BASE INFERIOR ---
        for (int i = 0; i <= segments; i++)
        {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            float x = cos(angle);
            float z = sin(angle);

            // POS
            cylVertices.push_back(x);
            cylVertices.push_back(0.0f);
            cylVertices.push_back(z);

            // NORMAL (hacia abajo)
            cylVertices.push_back(0.0f);
            cylVertices.push_back(-1.0f);
            cylVertices.push_back(0.0f);

            // UV
            cylVertices.push_back((x + 1.0f) * 0.5f);
            cylVertices.push_back((z + 1.0f) * 0.5f);
        }

        // --- BASE SUPERIOR ---
        for (int i = 0; i <= segments; i++)
        {
            float angle = (float)i / segments * 2.0f * 3.14159f;
            float x = cos(angle);
            float z = sin(angle);

            cylVertices.push_back(x);
            cylVertices.push_back(height);
            cylVertices.push_back(z);

            // NORMAL (hacia arriba)
            cylVertices.push_back(0.0f);
            cylVertices.push_back(1.0f);
            cylVertices.push_back(0.0f);

            cylVertices.push_back((x + 1.0f) * 0.5f);
            cylVertices.push_back((z + 1.0f) * 0.5f);
        }
        for (int i = 0; i < segments; i++)
        {
            int base0 = i;
            int base1 = i + 1;
            int top0 = i + (segments + 1);
            int top1 = i + (segments + 1) + 1;

            // Triángulo 1
            cylIndices.push_back(base0);
            cylIndices.push_back(top0);
            cylIndices.push_back(top1);

            // Triángulo 2
            cylIndices.push_back(base0);
            cylIndices.push_back(top1);
            cylIndices.push_back(base1);
        }

        for (int i = 1; i < segments; i++)
        {
            cylIndices.push_back(0);
            cylIndices.push_back(i);
            cylIndices.push_back(i + 1);
        }

        int topCenter = segments + 1;

        for (int i = 1; i < segments; i++)
        {
            cylIndices.push_back(topCenter);
            cylIndices.push_back(topCenter + i);
            cylIndices.push_back(topCenter + i + 1);
        }

        cylinderVAO = new VertexArray();
        cylinderVAO->Bind();

        cylinderVBO = new VertexBuffer(cylVertices.data(), cylVertices.size() * sizeof(float));
        cylinderVBO->Bind();

        cylinderIBO = new IndexBuffer(cylIndices.data(), cylIndices.size());
        cylinderIBO->Bind();

        cylinderVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        cylinderVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        cylinderVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        cylinderMesh.vao = cylinderVAO->GetID();
        cylinderMesh.indexCount = cylIndices.size();
        cylinderMesh.material = new Material();

        //Cone:
        std::vector<float> coneVertices;
        std::vector<uint32_t> coneIndices;

        int coneSegments = 32;

        // --- VÉRTICE DE LA PUNTA ---
        coneVertices.push_back(0.0f);  // x
        coneVertices.push_back(1.0f);  // y
        coneVertices.push_back(0.0f);  // z

        // normal de la punta (aproximada)
        coneVertices.push_back(0.0f);
        coneVertices.push_back(1.0f);
        coneVertices.push_back(0.0f);

        // UV
        coneVertices.push_back(0.5f);
        coneVertices.push_back(1.0f);

        // --- VÉRTICES DE LA BASE ---
        for (int i = 0; i <= coneSegments; i++)
        {
            float angle = (float)i / coneSegments * 2.0f * 3.14159f;
            float x = cos(angle);
            float z = sin(angle);

            // POS
            coneVertices.push_back(x);
            coneVertices.push_back(0.0f);
            coneVertices.push_back(z);

            // NORMAL (aproximada hacia afuera)
            coneVertices.push_back(x);
            coneVertices.push_back(0.5f);
            coneVertices.push_back(z);

            // UV
            coneVertices.push_back((x + 1.0f) * 0.5f);
            coneVertices.push_back((z + 1.0f) * 0.5f);
        }

        for (int i = 1; i <= coneSegments; i++)
        {
            coneIndices.push_back(0);        // punta
            coneIndices.push_back(i);
            coneIndices.push_back(i + 1);
        }
        for (int i = 1; i < coneSegments; i++)
        {
            coneIndices.push_back(1);        // primer vértice de la base
            coneIndices.push_back(i + 1);
            coneIndices.push_back(i);
        }

        coneVAO = new VertexArray();
        coneVAO->Bind();

        coneVBO = new VertexBuffer(coneVertices.data(), coneVertices.size() * sizeof(float));
        coneVBO->Bind();

        coneIBO = new IndexBuffer(coneIndices.data(), coneIndices.size());
        coneIBO->Bind();

        coneVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        coneVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        coneVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        coneMesh.vao = coneVAO->GetID();
        coneMesh.indexCount = coneIndices.size();
        coneMesh.material = new Material();

        //TORUS(Donut):
        std::vector<float> torusVertices;
        std::vector<uint32_t> torusIndices;

        int torusMajor = 32;   // segmentos alrededor del donut
        int torusMinor = 16;   // segmentos del grosor

        float majorRadius = 1.0f;   // radio grande
        float minorRadius = 0.3f;   // radio pequeño

        for (int i = 0; i <= torusMajor; i++)
        {
            float majorAngle = (float)i / torusMajor * 2.0f * 3.14159f;
            float cosMajor = cos(majorAngle);
            float sinMajor = sin(majorAngle);

            for (int j = 0; j <= torusMinor; j++)
            {
                float minorAngle = (float)j / torusMinor * 2.0f * 3.14159f;
                float cosMinor = cos(minorAngle);
                float sinMinor = sin(minorAngle);

                float x = (majorRadius + minorRadius * cosMinor) * cosMajor;
                float y = minorRadius * sinMinor;
                float z = (majorRadius + minorRadius * cosMinor) * sinMajor;

                // POS
                torusVertices.push_back(x);
                torusVertices.push_back(y);
                torusVertices.push_back(z);

                // NORMAL
                float nx = cosMajor * cosMinor;
                float ny = sinMinor;
                float nz = sinMajor * cosMinor;

                torusVertices.push_back(nx);
                torusVertices.push_back(ny);
                torusVertices.push_back(nz);

                // UV
                torusVertices.push_back((float)i / torusMajor);
                torusVertices.push_back((float)j / torusMinor);
            }
        }
        for (int i = 0; i < torusMajor; i++)
        {
            for (int j = 0; j < torusMinor; j++)
            {
                int first = i * (torusMinor + 1) + j;
                int second = first + torusMinor + 1;

                torusIndices.push_back(first);
                torusIndices.push_back(second);
                torusIndices.push_back(first + 1);

                torusIndices.push_back(second);
                torusIndices.push_back(second + 1);
                torusIndices.push_back(first + 1);
            }
        }
        torusVAO = new VertexArray();
        torusVAO->Bind();

        torusVBO = new VertexBuffer(torusVertices.data(), torusVertices.size() * sizeof(float));
        torusVBO->Bind();

        torusIBO = new IndexBuffer(torusIndices.data(), torusIndices.size());
        torusIBO->Bind();

        torusVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        torusVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        torusVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        torusMesh.vao = torusVAO->GetID();
        torusMesh.indexCount = torusIndices.size();
        torusMesh.material = new Material();

        
// TUBE (cilindro hueco)
// ===============================

        std::vector<float> tubeVertices;
        std::vector<uint32_t> tubeIndices;

        int seg = 32;          // segmentos alrededor
        float outerR = 1.0f;   // radio exterior
        float innerR = 0.7f;   // radio interior
        //float height = 1.5f;   // altura

        // --- VÉRTICES ---
        // Dos anillos: arriba y abajo
        for (int y = 0; y <= 1; y++)
        {
            float py = (y == 0 ? -height * 0.5f : height * 0.5f);

            for (int i = 0; i <= seg; i++)
            {
                float angle = (float)i / seg * 2.0f * 3.14159f;
                float c = cos(angle);
                float s = sin(angle);

                // EXTERIOR
                {
                    float px = outerR * c;
                    float pz = outerR * s;

                    glm::vec3 n = glm::normalize(glm::vec3(c, 0.0f, s));

                    tubeVertices.push_back(px);
                    tubeVertices.push_back(py);
                    tubeVertices.push_back(pz);

                    tubeVertices.push_back(n.x);
                    tubeVertices.push_back(n.y);
                    tubeVertices.push_back(n.z);

                    tubeVertices.push_back((float)i / seg);
                    tubeVertices.push_back((float)y);
                }

                // INTERIOR
                {
                    float px = innerR * c;
                    float pz = innerR * s;

                    glm::vec3 n = glm::normalize(glm::vec3(-c, 0.0f, -s)); // normal invertida

                    tubeVertices.push_back(px);
                    tubeVertices.push_back(py);
                    tubeVertices.push_back(pz);

                    tubeVertices.push_back(n.x);
                    tubeVertices.push_back(n.y);
                    tubeVertices.push_back(n.z);

                    tubeVertices.push_back((float)i / seg);
                    tubeVertices.push_back((float)y);
                }
            }
        }

        // --- ÍNDICES ---
        int ringVerts = (seg + 1) * 2; // exterior + interior

        for (int y = 0; y < 1; y++)
        {
            int base = y * ringVerts;

            for (int i = 0; i < seg; i++)
            {
                int i0 = base + i * 2;
                int i1 = base + (i + 1) * 2;
                int i2 = base + i * 2 + ringVerts;
                int i3 = base + (i + 1) * 2 + ringVerts;

                // Exterior
                tubeIndices.push_back(i0);
                tubeIndices.push_back(i2);
                tubeIndices.push_back(i1);

                tubeIndices.push_back(i1);
                tubeIndices.push_back(i2);
                tubeIndices.push_back(i3);

                // Interior
                tubeIndices.push_back(i0 + 1);
                tubeIndices.push_back(i1 + 1);
                tubeIndices.push_back(i2 + 1);

                tubeIndices.push_back(i1 + 1);
                tubeIndices.push_back(i3 + 1);
                tubeIndices.push_back(i2 + 1);
            }
        }
        tubeVAO = new VertexArray();
        tubeVAO->Bind();

        tubeVBO = new VertexBuffer(tubeVertices.data(), tubeVertices.size() * sizeof(float));
        tubeVBO->Bind();

        tubeIBO = new IndexBuffer(tubeIndices.data(), tubeIndices.size());
        tubeIBO->Bind();

        tubeVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        tubeVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        tubeVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        tubeMesh.vao = tubeVAO->GetID();
        tubeMesh.indexCount = tubeIndices.size();
        tubeMesh.material = new Material();
        tubeMesh.material->albedo = glm::vec3(1.0f);
        tubeMesh.material->albedoMap = diffuse;
        tubeMesh.material->normalMap = normalMap;

        // ===============================
// CAPSULE PERFECTA
// ===============================

        std::vector<float> capVertices;
        std::vector<uint32_t> capIndices;

        int segX = 32;   // segmentos alrededor
        int segY = 16;   // segmentos verticales hemisferio

        float radius = 0.5f;
        //float height = 1.0f;

        // ---------------------------------------------
        // HEMISFERIO SUPERIOR
        // ---------------------------------------------
        for (int y = 0; y <= segY; y++)
        {
            float v = (float)y / segY;
            float phi = v * (3.14159f / 2.0f); // 0 → 90°

            for (int x = 0; x <= segX; x++)
            {
                float u = (float)x / segX;
                float theta = u * 2.0f * 3.14159f;

                float nx = cos(theta) * sin(phi);
                float ny = cos(phi);
                float nz = sin(theta) * sin(phi);

                float px = nx * radius;
                float py = ny * radius + height * 0.5f;
                float pz = nz * radius;

                capVertices.push_back(px);
                capVertices.push_back(py);
                capVertices.push_back(pz);

                capVertices.push_back(nx);
                capVertices.push_back(ny);
                capVertices.push_back(nz);

                capVertices.push_back(u);
                capVertices.push_back(v);
            }
        }

        int topOffset = 0;
        int topCount = (segX + 1) * (segY + 1);

        // ---------------------------------------------
        // CILINDRO CENTRAL
        // ---------------------------------------------
        for (int y = 0; y <= segY; y++)
        {
            float v = (float)y / segY;
            float py = height * 0.5f - v * height;

            for (int x = 0; x <= segX; x++)
            {
                float u = (float)x / segX;
                float theta = u * 2.0f * 3.14159f;

                float nx = cos(theta);
                float nz = sin(theta);

                float px = nx * radius;
                float pz = nz * radius;

                capVertices.push_back(px);
                capVertices.push_back(py);
                capVertices.push_back(pz);

                capVertices.push_back(nx);
                capVertices.push_back(0.0f);
                capVertices.push_back(nz);

                capVertices.push_back(u);
                capVertices.push_back(v);
            }
        }

        int cylOffset = topCount;
        int cylCount = (segX + 1) * (segY + 1);

        // ---------------------------------------------
        // HEMISFERIO INFERIOR
        // ---------------------------------------------
        for (int y = 0; y <= segY; y++)
        {
            float v = (float)y / segY;
            float phi = v * (3.14159f / 2.0f) + (3.14159f / 2.0f); // 90° → 180°

            for (int x = 0; x <= segX; x++)
            {
                float u = (float)x / segX;
                float theta = u * 2.0f * 3.14159f;

                float nx = cos(theta) * sin(phi);
                float ny = cos(phi);
                float nz = sin(theta) * sin(phi);

                float px = nx * radius;
                float py = ny * radius - height * 0.5f;
                float pz = nz * radius;

                capVertices.push_back(px);
                capVertices.push_back(py);
                capVertices.push_back(pz);

                capVertices.push_back(nx);
                capVertices.push_back(ny);
                capVertices.push_back(nz);

                capVertices.push_back(u);
                capVertices.push_back(v);
            }
        }

        int bottomOffset = topCount + cylCount;

        // ---------------------------------------------
        // ÍNDICES (MISMO PATRÓN QUE SPHERE)
        // ---------------------------------------------
        auto addQuad = [&](int a, int b, int c, int d)
            {
                capIndices.push_back(a);
                capIndices.push_back(b);
                capIndices.push_back(c);

                capIndices.push_back(a);
                capIndices.push_back(c);
                capIndices.push_back(d);
            };

        // Superior
        for (int y = 0; y < segY; y++)
        {
            for (int x = 0; x < segX; x++)
            {
                int i0 = topOffset + y * (segX + 1) + x;
                int i1 = topOffset + (y + 1) * (segX + 1) + x;
                int i2 = topOffset + (y + 1) * (segX + 1) + (x + 1);
                int i3 = topOffset + y * (segX + 1) + (x + 1);

                addQuad(i0, i1, i2, i3);
            }
        }

        // Cilindro
        for (int y = 0; y < segY; y++)
        {
            for (int x = 0; x < segX; x++)
            {
                int i0 = cylOffset + y * (segX + 1) + x;
                int i1 = cylOffset + (y + 1) * (segX + 1) + x;
                int i2 = cylOffset + (y + 1) * (segX + 1) + (x + 1);
                int i3 = cylOffset + y * (segX + 1) + (x + 1);

                addQuad(i0, i1, i2, i3);
            }
        }

        // Inferior
        for (int y = 0; y < segY; y++)
        {
            for (int x = 0; x < segX; x++)
            {
                int i0 = bottomOffset + y * (segX + 1) + x;
                int i1 = bottomOffset + (y + 1) * (segX + 1) + x;
                int i2 = bottomOffset + (y + 1) * (segX + 1) + (x + 1);
                int i3 = bottomOffset + y * (segX + 1) + (x + 1);

                addQuad(i0, i1, i2, i3);
            }
        }

        capsuleVAO = new VertexArray();
        capsuleVAO->Bind();

        capsuleVBO = new VertexBuffer(capVertices.data(), capVertices.size() * sizeof(float));
        capsuleVBO->Bind();

        capsuleIBO = new IndexBuffer(capIndices.data(), capIndices.size());
        capsuleIBO->Bind();

        capsuleVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        capsuleVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        capsuleVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        capsuleMesh.vao = capsuleVAO->GetID();
        capsuleMesh.indexCount = capIndices.size();
        capsuleMesh.material = new Material();
        capsuleMesh.material->albedo = glm::vec3(1.0f);
        capsuleMesh.material->albedoMap = diffuse;
        capsuleMesh.material->normalMap = normalMap;

     // DOORFRAME (marco de puerta)
    // ===============================

        std::vector<float> dfVertices;
        std::vector<uint32_t> dfIndices;

        // Tamaño del marco
        float w = 1.0f;   // ancho interior (igual que la puerta)
        float h = 2.0f;   // alto interior
        float t = 0.1f;   // grosor del marco
        float bw = 0.15f; // ancho de las barras del marco

        // Helper para añadir cubos
        auto addCube = [&](float x0, float y0, float z0,
            float x1, float y1, float z1)
            {
                uint32_t base = dfVertices.size() / 8;

                float verts[] = {
                    // Front
                    x0,y0,z0, 0,0,-1, 0,0,
                    x1,y0,z0, 0,0,-1, 1,0,
                    x1,y1,z0, 0,0,-1, 1,1,
                    x0,y1,z0, 0,0,-1, 0,1,

                    // Back
                    x0,y0,z1, 0,0,1, 0,0,
                    x1,y0,z1, 0,0,1, 1,0,
                    x1,y1,z1, 0,0,1, 1,1,
                    x0,y1,z1, 0,0,1, 0,1,

                    // Left
                    x0,y0,z0, -1,0,0, 0,0,
                    x0,y1,z0, -1,0,0, 0,1,
                    x0,y1,z1, -1,0,0, 1,1,
                    x0,y0,z1, -1,0,0, 1,0,

                    // Right
                    x1,y0,z0, 1,0,0, 0,0,
                    x1,y1,z0, 1,0,0, 0,1,
                    x1,y1,z1, 1,0,0, 1,1,
                    x1,y0,z1, 1,0,0, 1,0,

                    // Top
                    x0,y1,z0, 0,1,0, 0,0,
                    x1,y1,z0, 0,1,0, 1,0,
                    x1,y1,z1, 0,1,0, 1,1,
                    x0,y1,z1, 0,1,0, 0,1,

                    // Bottom
                    x0,y0,z0, 0,-1,0, 0,0,
                    x1,y0,z0, 0,-1,0, 1,0,
                    x1,y0,z1, 0,-1,0, 1,1,
                    x0,y0,z1, 0,-1,0, 0,1,
                };

                dfVertices.insert(dfVertices.end(), verts, verts + 8 * 6 * 4);

                uint32_t inds[] = {
                    0,1,2, 2,3,0,
                    4,5,6, 6,7,4,
                    8,9,10, 10,11,8,
                    12,13,14, 14,15,12,
                    16,17,18, 18,19,16,
                    20,21,22, 22,23,20
                };

                for (int i = 0; i < 36; i++)
                    dfIndices.push_back(base + inds[i]);
            };

        // ---------------------------------------------
        // BARRA IZQUIERDA
        // ---------------------------------------------
        addCube(-w / 2 - bw, 0, -t / 2, -w / 2, h, t / 2);

        // ---------------------------------------------
        // BARRA DERECHA
        // ---------------------------------------------
        addCube(w / 2, 0, -t / 2, w / 2 + bw, h, t / 2);

        // ---------------------------------------------
        // BARRA SUPERIOR
        // ---------------------------------------------
        addCube(-w / 2 - bw, h, -t / 2, w / 2 + bw, h + bw, t / 2);

        // ---------------------------------------------
        // BARRA INFERIOR
        // ---------------------------------------------
        addCube(-w / 2 - bw, -bw, -t / 2, w / 2 + bw, 0, t / 2);

        doorFrameVAO = new VertexArray();
        doorFrameVAO->Bind();

        doorFrameVBO = new VertexBuffer(dfVertices.data(), dfVertices.size() * sizeof(float));
        doorFrameVBO->Bind();

        doorFrameIBO = new IndexBuffer(dfIndices.data(), dfIndices.size());
        doorFrameIBO->Bind();

        doorFrameVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        doorFrameVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        doorFrameVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        doorFrameMesh.vao = doorFrameVAO->GetID();
        doorFrameMesh.indexCount = dfIndices.size();
        doorFrameMesh.material = new Material();
        doorFrameMesh.material->albedo = glm::vec3(1.0f);
        doorFrameMesh.material->albedoMap = diffuse;
        doorFrameMesh.material->normalMap = normalMap;
        // ===============================
// DOOR (puerta 3D)
// ===============================

        std::vector<float> doorVertices;
        std::vector<uint32_t> doorIndices;

       
        
        // POS + NORMAL + UV
        float verts[] = {
            // Cara frontal
            -w / 2, 0, -t / 2,   0,0,-1,   0,0,
             w / 2, 0, -t / 2,   0,0,-1,   1,0,
             w / 2, h, -t / 2,   0,0,-1,   1,1,
            -w / 2, h, -t / 2,   0,0,-1,   0,1,

            // Cara trasera
            -w / 2, 0,  t / 2,   0,0,1,    0,0,
             w / 2, 0,  t / 2,   0,0,1,    1,0,
             w / 2, h,  t / 2,   0,0,1,    1,1,
            -w / 2, h,  t / 2,   0,0,1,    0,1,

            // Izquierda
            -w / 2, 0, -t / 2,  -1,0,0,    0,0,
            -w / 2, h, -t / 2,  -1,0,0,    0,1,
            -w / 2, h,  t / 2,  -1,0,0,    1,1,
            -w / 2, 0,  t / 2,  -1,0,0,    1,0,

            // Derecha
             w / 2, 0, -t / 2,   1,0,0,    0,0,
             w / 2, h, -t / 2,   1,0,0,    0,1,
             w / 2, h,  t / 2,   1,0,0,    1,1,
             w / 2, 0,  t / 2,   1,0,0,    1,0,

             // Arriba
             -w / 2, h, -t / 2,   0,1,0,    0,0,
              w / 2, h, -t / 2,   0,1,0,    1,0,
              w / 2, h,  t / 2,   0,1,0,    1,1,
             -w / 2, h,  t / 2,   0,1,0,    0,1,

             // Abajo
             -w / 2, 0, -t / 2,   0,-1,0,   0,0,
              w / 2, 0, -t / 2,   0,-1,0,   1,0,
              w / 2, 0,  t / 2,   0,-1,0,   1,1,
             -w / 2, 0,  t / 2,   0,-1,0,   0,1,
        };

        doorVertices.assign(verts, verts + sizeof(verts) / sizeof(float));

        uint32_t inds[] = {
            // frontal
            0,1,2, 2,3,0,
            // trasera
            4,5,6, 6,7,4,
            // izquierda
            8,9,10, 10,11,8,
            // derecha
            12,13,14, 14,15,12,
            // arriba
            16,17,18, 18,19,16,
            // abajo
            20,21,22, 22,23,20
        };

        doorIndices.assign(inds, inds + sizeof(inds) / sizeof(uint32_t));

        doorVAO = new VertexArray();
        doorVAO->Bind();

        doorVBO = new VertexBuffer(doorVertices.data(), doorVertices.size() * sizeof(float));
        doorVBO->Bind();

        doorIBO = new IndexBuffer(doorIndices.data(), doorIndices.size());
        doorIBO->Bind();

        doorVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        doorVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        doorVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        doorMesh.vao = doorVAO->GetID();
        doorMesh.indexCount = doorIndices.size();
        doorMesh.material = new Material();
        doorMesh.material->albedo = glm::vec3(1.0f);
        doorMesh.material->albedoMap = diffuse;
        doorMesh.material->normalMap = normalMap;

        // ===============================
        // REALISTIC BALL (icosphere)
        // ===============================

        std::vector<float> ballVertices;
        std::vector<uint32_t> ballIndices;

        //float radius = 0.5f;

        // Base icosahedron
        const float X = 0.525731f;
        const float Z = 0.850651f;

        glm::vec3 icoVerts[] = {
            {-X, 0, Z}, {X, 0, Z}, {-X, 0, -Z}, {X, 0, -Z},
            {0, Z, X}, {0, Z, -X}, {0, -Z, X}, {0, -Z, -X},
            {Z, X, 0}, {-Z, X, 0}, {Z, -X, 0}, {-Z, -X, 0}
        };

        uint32_t icoInds[] = {
            0,4,1, 0,9,4, 9,5,4, 4,5,8, 4,8,1,
            8,10,1, 8,3,10, 5,3,8, 5,2,3, 2,7,3,
            7,10,3, 7,6,10, 7,11,6, 11,0,6, 0,1,6,
            6,1,10, 9,0,11, 9,11,2, 9,2,5, 7,2,11
        };

        // Subdivide for realism
        auto subdivide = [&](int levels)
            {
                std::vector<glm::vec3> verts(icoVerts, icoVerts + 12);
                std::vector<uint32_t> inds(icoInds, icoInds + 60);

                for (int l = 0; l < levels; l++)
                {
                    std::map<std::pair<uint32_t, uint32_t>, uint32_t> midpointCache;
                    std::vector<uint32_t> newInds;

                    auto midpoint = [&](uint32_t a, uint32_t b)
                        {
                            auto key = std::minmax(a, b);
                            if (midpointCache.count(key)) return midpointCache[key];

                            glm::vec3 m = glm::normalize((verts[a] + verts[b]) * 0.5f);
                            verts.push_back(m);
                            uint32_t idx = verts.size() - 1;
                            midpointCache[key] = idx;
                            return idx;
                        };

                    for (int i = 0; i < inds.size(); i += 3)
                    {
                        uint32_t i0 = inds[i];
                        uint32_t i1 = inds[i + 1];
                        uint32_t i2 = inds[i + 2];

                        uint32_t a = midpoint(i0, i1);
                        uint32_t b = midpoint(i1, i2);
                        uint32_t c = midpoint(i2, i0);

                        newInds.insert(newInds.end(), { i0,a,c });
                        newInds.insert(newInds.end(), { i1,b,a });
                        newInds.insert(newInds.end(), { i2,c,b });
                        newInds.insert(newInds.end(), { a,b,c });
                    }

                    inds = newInds;
                }

                // Convert to your vertex format
                for (auto& v : verts)
                {
                    glm::vec3 p = v * radius;
                    glm::vec3 n = glm::normalize(v);
                    float u = atan2(n.z, n.x) / (2 * 3.14159f) + 0.5f;
                    float vTex = n.y * 0.5f + 0.5f;

                    ballVertices.insert(ballVertices.end(),
                        { p.x, p.y, p.z, n.x, n.y, n.z, u, vTex });
                }

                ballIndices = inds;
            };

        // Subdivide 2 times → pelota AAA
        subdivide(2);

        // ===============================
        // VAO / VBO / IBO (Realistic Ball)
        // ===============================

        ballVAO = new VertexArray();
        ballVAO->Bind();

        ballVBO = new VertexBuffer(ballVertices.data(), ballVertices.size() * sizeof(float));
        ballVBO->Bind();

        ballIBO = new IndexBuffer(ballIndices.data(), ballIndices.size());
        ballIBO->Bind();

        // POS (3) + NORMAL (3) + UV (2) = stride 8 floats
        ballVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);                 // posición
        ballVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
        ballVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV

        ballMesh.vao = ballVAO->GetID();
        ballMesh.indexCount = ballIndices.size();
        ballMesh.material = new Material();
        ballMesh.material->albedo = glm::vec3(1.0f);
        ballMesh.material->albedoMap = diffuse;
        ballMesh.material->normalMap = normalMap;


       
        // ===============================
// LADDER (escalera de mano)
// ===============================

        std::vector<float> ladderVertices;
        std::vector<uint32_t> ladderIndices;

        float ladderHeights = 2.5f;
        float ladderWidths = 0.4f;
        float barThicknesss = 0.05f;
        int stepss = 8;
        float stepDepthss = 0.1f;
        float stepThicknesss = 0.03f;

        // Helper para añadir cubos (SIN glm::vec3)
        auto pushCube = [&](float x0, float y0, float z0,
            float x1, float y1, float z1)
            {
                uint32_t base = ladderVertices.size() / 8;

                float verts[] = {
                    // Front
                    x0,y0,z0, 0,0,-1, 0,0,
                    x1,y0,z0, 0,0,-1, 1,0,
                    x1,y1,z0, 0,0,-1, 1,1,
                    x0,y1,z0, 0,0,-1, 0,1,

                    // Back
                    x0,y0,z1, 0,0,1, 0,0,
                    x1,y0,z1, 0,0,1, 1,0,
                    x1,y1,z1, 0,0,1, 1,1,
                    x0,y1,z1, 0,0,1, 0,1,

                    // Left
                    x0,y0,z0, -1,0,0, 0,0,
                    x0,y1,z0, -1,0,0, 0,1,
                    x0,y1,z1, -1,0,0, 1,1,
                    x0,y0,z1, -1,0,0, 1,0,

                    // Right
                    x1,y0,z0, 1,0,0, 0,0,
                    x1,y1,z0, 1,0,0, 0,1,
                    x1,y1,z1, 1,0,0, 1,1,
                    x1,y0,z1, 1,0,0, 1,0,

                    // Top
                    x0,y1,z0, 0,1,0, 0,0,
                    x1,y1,z0, 0,1,0, 1,0,
                    x1,y1,z1, 0,1,0, 1,1,
                    x0,y1,z1, 0,1,0, 0,1,

                    // Bottom
                    x0,y0,z0, 0,-1,0, 0,0,
                    x1,y0,z0, 0,-1,0, 1,0,
                    x1,y0,z1, 0,-1,0, 1,1,
                    x0,y0,z1, 0,-1,0, 0,1,
                };

                ladderVertices.insert(ladderVertices.end(), verts, verts + 8 * 6 * 4);

                uint32_t inds[] = {
                    0,1,2, 2,3,0,
                    4,5,6, 6,7,4,
                    8,9,10, 10,11,8,
                    12,13,14, 14,15,12,
                    16,17,18, 18,19,16,
                    20,21,22, 22,23,20
                };

                for (int i = 0; i < 36; i++)
                    ladderIndices.push_back(base + inds[i]);
            };

        // ===============================
        // LARGUEROS (los dos palos verticales)
        // ===============================

        pushCube(0, 0, 0,
            barThicknesss, ladderHeights, barThicknesss);

        pushCube(ladderWidths - barThicknesss, 0, 0,
            ladderWidths, ladderHeights, barThicknesss);

        // ===============================
        // PELDAÑOS
        // ===============================

        for (int i = 0; i < stepss; i++)
        {
            float y = (ladderHeights / (stepss + 1)) * (i + 1);

            pushCube(barThicknesss, y, 0,
                ladderWidths - barThicknesss, y + stepThicknesss, stepDepthss);
        }


        ladderVAO = new VertexArray();
        ladderVAO->Bind();

        ladderVBO = new VertexBuffer(ladderVertices.data(), ladderVertices.size() * sizeof(float));
        ladderVBO->Bind();

        ladderIBO = new IndexBuffer(ladderIndices.data(), ladderIndices.size());
        ladderIBO->Bind();

        ladderVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);
        ladderVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        ladderVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        ladderMesh.vao = ladderVAO->GetID();
        ladderMesh.indexCount = ladderIndices.size();
        ladderMesh.material = new Material();
        ladderMesh.material->albedo = glm::vec3(1.0f);
        ladderMesh.material->albedoMap = diffuse;
        ladderMesh.material->normalMap = normalMap;
        


        //Stair:
        auto pushCubes = [&](float x0, float y0, float z0,
            float x1, float y1, float z1,
            std::vector<float>& verts,
            std::vector<uint32_t>& inds)
            {
                uint32_t base = verts.size() / 8;

                float cubeVerts[] = {
                    // Front
                    x0,y0,z0, 0,0,-1, 0,0,
                    x1,y0,z0, 0,0,-1, 1,0,
                    x1,y1,z0, 0,0,-1, 1,1,
                    x0,y1,z0, 0,0,-1, 0,1,

                    // Back
                    x0,y0,z1, 0,0,1, 0,0,
                    x1,y0,z1, 0,0,1, 1,0,
                    x1,y1,z1, 0,0,1, 1,1,
                    x0,y1,z1, 0,0,1, 0,1,

                    // Left
                    x0,y0,z0, -1,0,0, 0,0,
                    x0,y1,z0, -1,0,0, 0,1,
                    x0,y1,z1, -1,0,0, 1,1,
                    x0,y0,z1, -1,0,0, 1,0,

                    // Right
                    x1,y0,z0, 1,0,0, 0,0,
                    x1,y1,z0, 1,0,0, 0,1,
                    x1,y1,z1, 1,0,0, 1,1,
                    x1,y0,z1, 1,0,0, 1,0,

                    // Top
                    x0,y1,z0, 0,1,0, 0,0,
                    x1,y1,z0, 0,1,0, 1,0,
                    x1,y1,z1, 0,1,0, 1,1,
                    x0,y1,z1, 0,1,0, 0,1,

                    // Bottom
                    x0,y0,z0, 0,-1,0, 0,0,
                    x1,y0,z0, 0,-1,0, 1,0,
                    x1,y0,z1, 0,-1,0, 1,1,
                    x0,y0,z1, 0,-1,0, 0,1,
                };

                verts.insert(verts.end(), cubeVerts, cubeVerts + 8 * 6 * 4);

                uint32_t cubeInds[] = {
                    0,1,2, 2,3,0,
                    4,5,6, 6,7,4,
                    8,9,10, 10,11,8,
                    12,13,14, 14,15,12,
                    16,17,18, 18,19,16,
                    20,21,22, 22,23,20
                };

                for (int i = 0; i < 36; i++)
                    inds.push_back(base + cubeInds[i]);
            };

        std::vector<float> stairVerts;
        std::vector<uint32_t> stairInds;

        // Parámetros
        float stepWidth = 1.0f;
        float stepDepth = 0.30f;
        float stepHeight = 0.18f;
        float stepThickness = 0.05f;

        int steps1 = 7;
        int steps2 = 7;

        float landingSize = 1.2f;

        // ===============================
        // TRAMO 1
        // ===============================

        for (int i = 0; i < steps1; i++)
        {
            float y = i * stepHeight;
            float z = i * stepDepth;

            pushCubes(0, y, z,
                stepWidth, y + stepThickness, z + stepDepth,
                stairVerts, stairInds);
        }

        // ===============================
        // DESCANSILLO
        // ===============================

        float landingY = steps1 * stepHeight;
        float landingZ = steps1 * stepDepth;

        pushCubes(0, landingY, landingZ,
            stepWidth, landingY + stepThickness, landingZ + landingSize,
            stairVerts, stairInds);

        // ===============================
        // TRAMO 2 (giro 90 grados)
        // ===============================

        for (int i = 0; i < steps2; i++)
        {
            float y = landingY + i * stepHeight;
            float x = i * stepDepth;

            pushCubes(x, y, landingZ + landingSize,
                x + stepDepth, y + stepThickness, landingZ + landingSize + stepWidth,
                stairVerts, stairInds);
        }

        // ===============================
        // BARANDILLA DE CRISTAL
        // ===============================

        float glassThickness = 0.02f;
        float glassHeight = 1.0f;

        // tramo 1
        pushCubes(stepWidth + 0.05f, 0, 0,
            stepWidth + 0.05f + glassThickness, glassHeight, landingZ,
            stairVerts, stairInds);

        // tramo 2
        pushCubes(0, landingY,
            landingZ + landingSize + stepWidth + 0.05f,
            glassThickness, landingY + glassHeight, landingZ + landingSize,
            stairVerts, stairInds);

        modernStairVAO = new VertexArray();
        modernStairVAO->Bind();

        modernStairVBO = new VertexBuffer(stairVerts.data(), stairVerts.size() * sizeof(float));
        modernStairVBO->Bind();

        modernStairIBO = new IndexBuffer(stairInds.data(), stairInds.size());
        modernStairIBO->Bind();

        modernStairVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);
        modernStairVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        modernStairVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        modernStairMesh.vao = modernStairVAO->GetID();
        modernStairMesh.indexCount = stairInds.size();
        modernStairMesh.material =  new Material();
        modernStairMesh.material->albedo = glm::vec3(1.0f);
        modernStairMesh.material->albedoMap = diffuse;
        modernStairMesh.material->normalMap = normalMap;


        auto pushCubess = [&](float x0, float y0, float z0,
            float x1, float y1, float z1,
            std::vector<float>& verts,
            std::vector<uint32_t>& inds)
            {
                uint32_t base = verts.size() / 8;

                float v[] = {
                    // Front
                    x0,y0,z0, 0,0,-1, 0,0,
                    x1,y0,z0, 0,0,-1, 1,0,
                    x1,y1,z0, 0,0,-1, 1,1,
                    x0,y1,z0, 0,0,-1, 0,1,

                    // Back
                    x0,y0,z1, 0,0,1, 0,0,
                    x1,y0,z1, 0,0,1, 1,0,
                    x1,y1,z1, 0,0,1, 1,1,
                    x0,y1,z1, 0,0,1, 0,1,

                    // Left
                    x0,y0,z0, -1,0,0, 0,0,
                    x0,y1,z0, -1,0,0, 0,1,
                    x0,y1,z1, -1,0,0, 1,1,
                    x0,y0,z1, -1,0,0, 1,0,

                    // Right
                    x1,y0,z0, 1,0,0, 0,0,
                    x1,y1,z0, 1,0,0, 0,1,
                    x1,y1,z1, 1,0,0, 1,1,
                    x1,y0,z1, 1,0,0, 1,0,

                    // Top
                    x0,y1,z0, 0,1,0, 0,0,
                    x1,y1,z0, 0,1,0, 1,0,
                    x1,y1,z1, 0,1,0, 1,1,
                    x0,y1,z1, 0,1,0, 0,1,

                    // Bottom
                    x0,y0,z0, 0,-1,0, 0,0,
                    x1,y0,z0, 0,-1,0, 1,0,
                    x1,y0,z1, 0,-1,0, 1,1,
                    x0,y0,z1, 0,-1,0, 0,1,
                };

                verts.insert(verts.end(), v, v + 8 * 6 * 4);

                uint32_t idx[] = {
                    0,1,2, 2,3,0,
                    4,5,6, 6,7,4,
                    8,9,10, 10,11,8,
                    12,13,14, 14,15,12,
                    16,17,18, 18,19,16,
                    20,21,22, 22,23,20
                };

                for (int i = 0; i < 36; i++)
                    inds.push_back(base + idx[i]);
            };

        // ===============================
 // TABLE GEOMETRY (STATIC LIKE TORUS)
 // ===============================

 // TABLERO: cubo de 2x0.2x2
 // PATAS: cubos de 0.2x1x0.2

        float tableVertices[] = {
            // ===== TABLERO =====
            // posX posY posZ   normalX normalY normalZ   u   v
            -1.0f, 0.0f, -1.0f,   0,0,-1,   0,0,
             1.0f, 0.0f, -1.0f,   0,0,-1,   1,0,
             1.0f, 0.2f, -1.0f,   0,0,-1,   1,1,
            -1.0f, 0.2f, -1.0f,   0,0,-1,   0,1,

            -1.0f, 0.0f,  1.0f,   0,0,1,    0,0,
             1.0f, 0.0f,  1.0f,   0,0,1,    1,0,
             1.0f, 0.2f,  1.0f,   0,0,1,    1,1,
            -1.0f, 0.2f,  1.0f,   0,0,1,    0,1,

            // ===== PATA 1 =====
            -1.0f, -1.0f, -1.0f,   0,0,-1,   0,0,
            -0.8f, -1.0f, -1.0f,   0,0,-1,   1,0,
            -0.8f,  0.0f, -1.0f,   0,0,-1,   1,1,
            -1.0f,  0.0f, -1.0f,   0,0,-1,   0,1,

            -1.0f, -1.0f, -0.8f,   0,0,1,    0,0,
            -0.8f, -1.0f, -0.8f,   0,0,1,    1,0,
            -0.8f,  0.0f, -0.8f,   0,0,1,    1,1,
            -1.0f,  0.0f, -0.8f,   0,0,1,    0,1,

            // ===== PATA 2 =====
             0.8f, -1.0f, -1.0f,   0,0,-1,   0,0,
             1.0f, -1.0f, -1.0f,   0,0,-1,   1,0,
             1.0f,  0.0f, -1.0f,   0,0,-1,   1,1,
             0.8f,  0.0f, -1.0f,   0,0,-1,   0,1,

             0.8f, -1.0f, -0.8f,   0,0,1,    0,0,
             1.0f, -1.0f, -0.8f,   0,0,1,    1,0,
             1.0f,  0.0f, -0.8f,   0,0,1,    1,1,
             0.8f,  0.0f, -0.8f,   0,0,1,    0,1,

             // ===== PATA 3 =====
             -1.0f, -1.0f,  0.8f,   0,0,-1,   0,0,
             -0.8f, -1.0f,  0.8f,   0,0,-1,   1,0,
             -0.8f,  0.0f,  0.8f,   0,0,-1,   1,1,
             -1.0f,  0.0f,  0.8f,   0,0,-1,   0,1,

             -1.0f, -1.0f,  1.0f,   0,0,1,    0,0,
             -0.8f, -1.0f,  1.0f,   0,0,1,    1,0,
             -0.8f,  0.0f,  1.0f,   0,0,1,    1,1,
             -1.0f,  0.0f,  1.0f,   0,0,1,    0,1,

             // ===== PATA 4 =====
              0.8f, -1.0f,  0.8f,   0,0,-1,   0,0,
              1.0f, -1.0f,  0.8f,   0,0,-1,   1,0,
              1.0f,  0.0f,  0.8f,   0,0,-1,   1,1,
              0.8f,  0.0f,  0.8f,   0,0,-1,   0,1,

              0.8f, -1.0f,  1.0f,   0,0,1,    0,0,
              1.0f, -1.0f,  1.0f,   0,0,1,    1,0,
              1.0f,  0.0f,  1.0f,   0,0,1,    1,1,
              0.8f,  0.0f,  1.0f,   0,0,1,    0,1,
        };

        // Cada cubo tiene 36 índices
        uint32_t tableIndices[36 * 5]; // tablero + 4 patas
        int idx = 0;

        for (int base = 0; base < 5; base++)
        {
            uint32_t b = base * 8;
            uint32_t inds[] = {
                b + 0, b + 1, b + 2, b + 2, b + 3, b + 0,
                b + 4, b + 5, b + 6, b + 6, b + 7, b + 4,
                b + 0, b + 1, b + 5, b + 5, b + 4, b + 0,
                b + 1, b + 2, b + 6, b + 6, b + 5, b + 1,
                b + 2, b + 3, b + 7, b + 7, b + 6, b + 2,
                b + 3, b + 0, b + 4, b + 4, b + 7, b + 3
            };
            memcpy(&tableIndices[idx], inds, sizeof(inds));
            idx += 36;
        }

        tableVAO = new VertexArray();
        tableVAO->Bind();

        tableVBO = new VertexBuffer(tableVertices, sizeof(tableVertices));
        tableVBO->Bind();

        tableIBO = new IndexBuffer(tableIndices, 36 * 5);
        tableIBO->Bind();

        tableVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);
        tableVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        tableVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        tableMesh.vao = tableVAO->GetID();
        tableMesh.indexCount = 36 * 5;
        tableMesh.material = new Material();
        tableMesh.material->albedo = glm::vec3(0.6f, 0.3f, 0.1f); // color madera
        tableMesh.material->albedoMap = nullptr;
        tableMesh.material->normalMap = nullptr;


        //Knife:
        // posX posY posZ   normalX normalY normalZ   u   v
        float knifeVertices[] =
        {
            // ===== HOJA (prisma 3D) =====
            -0.1f, 0.0f, -0.02f,   0,0,-1,   0,0,
             0.1f, 0.0f, -0.02f,   0,0,-1,   1,0,
             0.1f, 1.5f, -0.02f,   0,0,-1,   1,1,
            -0.1f, 1.5f, -0.02f,   0,0,-1,   0,1,

            -0.1f, 0.0f,  0.02f,   0,0,1,    0,0,
             0.1f, 0.0f,  0.02f,   0,0,1,    1,0,
             0.1f, 1.5f,  0.02f,   0,0,1,    1,1,
            -0.1f, 1.5f,  0.02f,   0,0,1,    0,1,

            // ===== MANGO (prisma 3D) =====
            -0.2f, -0.5f, -0.05f,  0,0,-1,   0,0,
             0.2f, -0.5f, -0.05f,  0,0,-1,   1,0,
             0.2f,  0.0f, -0.05f,  0,0,-1,   1,1,
            -0.2f,  0.0f, -0.05f,  0,0,-1,   0,1,

            -0.2f, -0.5f,  0.05f,  0,0,1,    0,0,
             0.2f, -0.5f,  0.05f,  0,0,1,    1,0,
             0.2f,  0.0f,  0.05f,  0,0,1,    1,1,
            -0.2f,  0.0f,  0.05f,  0,0,1,    0,1,
        };
        uint32_t knifeIndices[] =
        {
            // HOJA
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            0,1,5, 5,4,0,
            1,2,6, 6,5,1,
            2,3,7, 7,6,2,
            3,0,4, 4,7,3,

            // MANGO (offset 8)
            8,9,10, 10,11,8,
            12,13,14, 14,15,12,
            8,9,13, 13,12,8,
            9,10,14, 14,13,9,
            10,11,15, 15,14,10,
            11,8,12, 12,15,11
        };
        knifeVAO = new VertexArray();
        knifeVAO->Bind();

        knifeVBO = new VertexBuffer(knifeVertices, sizeof(knifeVertices));
        knifeVBO->Bind();

        knifeIBO = new IndexBuffer(knifeIndices, sizeof(knifeIndices) / sizeof(uint32_t));
        knifeIBO->Bind();

        knifeVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);
        knifeVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        knifeVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        knifeMesh.vao = knifeVAO->GetID();
        knifeMesh.indexCount = sizeof(knifeIndices) / sizeof(uint32_t);
        knifeMesh.material = new Material();
        knifeMesh.material->albedo = glm::vec3(0.8f, 0.8f, 0.8f);
        knifeMesh.material->albedoMap = nullptr;
        knifeMesh.material->normalMap = nullptr;

        

  

          uint32_t cubeIndices[] = {
            0,1,2, 2,3,0,
            4,5,6, 6,7,4,
            0,4,7, 7,3,0,
            1,5,6, 6,2,1,
            3,2,6, 6,7,3,
            0,1,5, 5,4,0
        };

        // Suelo: VAO local, VBO/IBO en miembros
        planeVAO->Bind();


        planeVBO = new VertexBuffer(planeVertices, sizeof(planeVertices));
        planeVBO->Bind();

        planeIBO = new IndexBuffer(planeIndices, 6);

        planeVAO->AddVertexBuffer(0, 3, 14 * sizeof(float), (void*)0);
        planeVAO->AddVertexBuffer(1, 3, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        planeVAO->AddVertexBuffer(2, 2, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        planeVAO->AddVertexBuffer(3, 3, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        planeVAO->AddVertexBuffer(4, 3, 14 * sizeof(float), (void*)(11 * sizeof(float)));


        // Cubo: VAO local, VBO/IBO en miembros
        cubeVAO->Bind();


        cubeVBO = new VertexBuffer(cubeVertices, sizeof(cubeVertices));
        cubeVBO->Bind();

        cubeIBO = new IndexBuffer(cubeIndices, 36);

        cubeVAO->AddVertexBuffer(0, 3, 8 * sizeof(float), (void*)0);
        cubeVAO->AddVertexBuffer(1, 3, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        cubeVAO->AddVertexBuffer(2, 2, 8 * sizeof(float), (void*)(6 * sizeof(float)));

        // Meshes
        cubeMesh.vao = cubeVAO->GetID();
        cubeMesh.indexCount = 36;

        cubeMat = new Material();
        cubeMat->albedo = glm::vec3(1.0f, 0.2f, 0.2f);
        cubeMat->albedoMap = diffuse;
        cubeMat->normalMap = normalMap;
        cubeMesh.material = cubeMat;


        //Material:
        cubeMesh.material = new Material();
        cubeMesh.material->albedo = glm::vec3(1.0f);
        cubeMesh.material->albedoMap = diffuse;
        cubeMesh.material->normalMap = normalMap;


        planeMesh.material = new Material();
        planeMesh.material->albedo = glm::vec3(1.0f);
        planeMesh.material->albedoMap = diffuse;
        planeMesh.material->normalMap = normalMap;

        triangleMesh.material = new Material();
        triangleMesh.material->albedo = glm::vec3(1.0f);
        triangleMesh.material->albedoMap = diffuse;
        triangleMesh.material->normalMap = normalMap;


        sphereMesh.material = new Material();
        sphereMesh.material->albedo = glm::vec3(1.0f);
        sphereMesh.material->albedoMap = diffuse;
        sphereMesh.material->normalMap = normalMap;


        cylinderMesh.material = new Material();
        cylinderMesh.material->albedo = glm::vec3(1.0f);
        cylinderMesh.material->albedoMap = diffuse;
        cylinderMesh.material->normalMap = normalMap;


        coneMesh.material = new Material();
        coneMesh.material->albedo = glm::vec3(1.0f);
        coneMesh.material->albedoMap = diffuse;
        coneMesh.material->normalMap = normalMap;


        torusMesh.material = new Material();
        torusMesh.material->albedo = glm::vec3(1.0f);
        torusMesh.material->albedoMap = diffuse;
        torusMesh.material->normalMap = normalMap;


        pyramidMesh.material = new Material();
        pyramidMesh.material->albedo = glm::vec3(1.0f);
        pyramidMesh.material->albedoMap = diffuse;
        pyramidMesh.material->normalMap = normalMap;


        circleMesh.material = new Material();
        circleMesh.material->albedo = glm::vec3(1.0f);
        circleMesh.material->albedoMap = diffuse;
        circleMesh.material->normalMap = normalMap;


        quadMesh.material = new Material();
        quadMesh.material->albedo = glm::vec3(1.0f);
        quadMesh.material->albedoMap = diffuse;
        quadMesh.material->normalMap = normalMap;

        planeMesh.vao = planeVAO->GetID();
        planeMesh.indexCount = 6;

        Material* planeMat = new Material();
        planeMat->albedo = glm::vec3(1.0f);
        planeMat->albedoMap = diffuse;
        planeMat->normalMap = normalMap;
        planeMesh.material = planeMat;

        lightingShader.Bind();
        lightingShader.SetInt("u_Texture", 0);

        
        
       // Shadow map FBO
        glGenFramebuffers(1, &depthMapFBO);

        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1,1,1,1 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // FBO final
        glGenFramebuffers(1, &finalFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, finalFBO);

        glGenTextures(1, &finalColor);
        glBindTexture(GL_TEXTURE_2D, finalColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, finalColor, 0);

        glGenTextures(1, &finalDepth);
        glBindTexture(GL_TEXTURE_2D, finalDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1280, 720, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, finalDepth, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "FINAL FBO ERROR" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Textura ruido
        const int NOISE_SIZE = 256;
        std::vector<unsigned char> noiseData(NOISE_SIZE * NOISE_SIZE);

        for (int y = 0; y < NOISE_SIZE; y++)
        {
            for (int x = 0; x < NOISE_SIZE; x++)
            {
                float nx = x / 64.0f;
                float ny = y / 64.0f;

                float p = fbm(nx, ny);
                float w = worley2D(nx * 0.5f, ny * 0.5f);

                float final = p * 0.6f + (1.0f - w) * 0.4f;
                final = pow(final, 1.5f);

                noiseData[y * NOISE_SIZE + x] = (unsigned char)(final * 255.0f);
            }
        }

        glGenTextures(1, &noiseTextureID);
        glBindTexture(GL_TEXTURE_2D, noiseTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, NOISE_SIZE, NOISE_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, noiseData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Bloom FBO
        glGenFramebuffers(1, &bloomFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, bloomFBO);

        glGenTextures(1, &bloomColor);
        glBindTexture(GL_TEXTURE_2D, bloomColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomColor, 0);

        GLenum bloomDrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, bloomDrawBuffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "BLOOM FBO ERROR" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Ping-pong FBOs
        glGenFramebuffers(2, pingpongFBO);
        glGenTextures(2, pingpongColor);

        for (int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);

            glBindTexture(GL_TEXTURE_2D, pingpongColor[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 1280, 720, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColor[i], 0);

            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, drawBuffers);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
     }
}



void Application::Run()
{
    while (!window->ShouldClose())
    {
        window->PollEvents();
        Time::Update();
        Input::Update();

       



        float dt = Time::GetDeltaTime();
        if (isPlaying)
        {
            int axesCount;
            const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);

            if (axes && axesCount >= 2)
            {
                float lx = axes[0]; // izquierda/derecha
                float ly = axes[1]; // adelante/atrás

                float speed = 5.0f * dt;

                camera->Position += camera->Front * (-ly) * speed;
                camera->Position += camera->Right * (lx)*speed;

            }

            if (axes && axesCount >= 4)
            {
                float rx = axes[2]; // rotación horizontal
                float ry = axes[3]; // rotación vertical

                float sensitivity = 2.0f;

                camera->Yaw += rx * sensitivity;
                camera->Pitch -= ry * sensitivity;

                camera->UpdateCameraVectors();
            }
        }



        Update(dt);
        Render();

        window->SwapBuffers();
    }
}

void Application::Update(float dt)
{
    bool userIsMovingCube = false;

    float gravity = -9.81f;
    float floorY = 0.0f;

    for (auto& e : scene.entities)
    {
        if (!e.isBall) continue;

        float dt = Time::GetDeltaTime();

        // Gravedad
        e.velocity.y += gravity * dt;

        // Integración
        e.position += e.velocity * dt;

        // Colisión con el suelo
        if (e.position.y - e.radius < floorY)
        {
            e.position.y = floorY + e.radius;

            // Rebote realista
            e.velocity.y *= -e.elasticity;

            // Fricción horizontal
            e.velocity.x *= e.friction;
            e.velocity.z *= e.friction;

            // Detener si casi no rebota
            if (fabs(e.velocity.y) < 0.1f)
                e.velocity.y = 0;
        }
    }





    if (!ImGuizmo::IsUsing())
    {
        bool moving =
            Input::IsKeyPressed(KEY_W) ||
            Input::IsKeyPressed(KEY_S) ||
            Input::IsKeyPressed(KEY_A) ||
            Input::IsKeyPressed(KEY_D);

        if (moving)
        {
            glfwSetInputMode(window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            if (isPlaying == false)
            { 
            camera->ProcessKeyboard(
                dt,
                Input::IsKeyPressed(KEY_W),
                Input::IsKeyPressed(KEY_S),
                Input::IsKeyPressed(KEY_A),
                Input::IsKeyPressed(KEY_D)
            );

            camera->ProcessMouse(Input::GetMouseDeltaX(), Input::GetMouseDeltaY());
           }
      }
        else
        {
            glfwSetInputMode(window->GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
  }





    // === GRAVEDAD ===
    // // === CORRECCIÓN DE COLISIÓN CON EL SUELO ===
    // Detectar si el usuario está manipulando algo en el editor
    userIsMovingCube =
        ImGui::IsAnyItemActive() || ImGuizmo::IsUsing();

    // === GRAVEDAD SOLO SI NO ESTÁS MOVIENDO EL CUBO ===
    if (!userIsMovingCube)
    {
        for (auto& e : scene.entities)
        {
            if (e.mesh == &cubeMesh)
            {
                e.velocity.y -= 9.8f * dt;     // gravedad
                e.position += e.velocity * dt; // aplicar movimiento
            }
        }





    }

   

    




 
    // === CORRECCIÓN DE COLISIÓN CON EL SUELO + REBOTE ===
    for (auto& e : scene.entities)
    {
        if (e.mesh == &cubeMesh)
        {
            float bottom = e.position.y + e.colliderMin.y * e.scale.y;

            if (bottom < 0.0f) // suelo en Y=0
            {
                float penetration = 0.0f - bottom;
                e.position.y += penetration;

                // --- REBOTE ESTILO UNREAL ---
                float impactSpeed = fabs(e.velocity.y);
                float restitution = glm::clamp(impactSpeed / 20.0f, 0.1f, 0.9f);
                e.velocity.y = impactSpeed * restitution;

                // Si el rebote es muy pequeño, detenerlo
                if (fabs(e.velocity.y) < 0.5f)
                    e.velocity.y = 0.0f;
            }
        }
    }

    // === DETECCIÓN DE COLISIONES ENTRE ENTIDADES ===
    for (int i = 0; i < scene.entities.size(); i++)
    {
        for (int j = i + 1; j < scene.entities.size(); j++)
        {
            if (CheckCollision(scene.entities[i], scene.entities[j]))
            {
                std::cout << "COLISION DETECTADA entre " << i << " y " << j << std::endl;

                glm::vec3 aMin = scene.entities[i].position + scene.entities[i].colliderMin * scene.entities[i].scale;
                glm::vec3 aMax = scene.entities[i].position + scene.entities[i].colliderMax * scene.entities[i].scale;

                glm::vec3 bMin = scene.entities[j].position + scene.entities[j].colliderMin * scene.entities[j].scale;
                glm::vec3 bMax = scene.entities[j].position + scene.entities[j].colliderMax * scene.entities[j].scale;

                // Penetración en cada eje
                float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
                float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
                float overlapZ = std::min(aMax.z, bMax.z) - std::max(aMin.z, bMin.z);

                // Elegir el eje con menor penetración
                if (overlapX < overlapY && overlapX < overlapZ)
                {
                    float push = overlapX * 0.5f; // mitad para cada cubo

                    if (scene.entities[i].position.x < scene.entities[j].position.x)
                    {
                        scene.entities[i].position.x -= push;
                        scene.entities[j].position.x += push;
                    }
                    else
                    {
                        scene.entities[i].position.x += push;
                        scene.entities[j].position.x -= push;
                    }
                }
                else if (overlapY < overlapX && overlapY < overlapZ)
                {
                    float push = overlapY * 0.5f;

                    if (scene.entities[i].position.y < scene.entities[j].position.y)
                    {
                        scene.entities[i].position.y -= push;
                        scene.entities[j].position.y += push;
                    }
                    else
                    {
                        scene.entities[i].position.y += push;
                        scene.entities[j].position.y -= push;
                    }

                    // Si A está cayendo sobre B, detener su velocidad
                    if (scene.entities[i].velocity.y < 0.0f &&
                        scene.entities[i].position.y > scene.entities[j].position.y)
                    {
                        scene.entities[i].velocity.y = 0.0f;
                    }

                }
                else
                {
                    float push = overlapZ * 0.5f;

                    if (scene.entities[i].position.z < scene.entities[j].position.z)
                    {
                        scene.entities[i].position.z -= push;
                        scene.entities[j].position.z += push;
                    }
                    else
                    {
                        scene.entities[i].position.z += push;
                        scene.entities[j].position.z -= push;
                    }


                }
             }
          }
    }

    // === CORRECCIÓN DE COLISIÓN CON EL SUELO ===
    for (auto& e : scene.entities)
    {
        // Si el cubo está por debajo del suelo, lo subimos
        if (e.position.y < 0.5f)
        {
            e.position.y = 0.5f;
        }
    }


    // === EVENTOS DE VOZ SAPI ===
    SPEVENT event;
    ULONG fetched = 0;

    // === EVENTOS DE VOZ SAPI ===
    if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
    {
        SPEVENT event;
        ULONG fetched = 0;

        while (context->GetEvents(1, &event, &fetched) == S_OK)
        {
            if (event.eEventId == SPEI_RECOGNITION)
            {
                ISpRecoResult* result =
                    reinterpret_cast<ISpRecoResult*>(event.lParam);

                SPPHRASE* pPhrase = nullptr;
                if (SUCCEEDED(result->GetPhrase(&pPhrase)))
                {
                    std::wstring text(pPhrase->pProperties->pszValue);

                    lastVoiceCommand = text;
                    ProcessVoiceCommand(text);
                }
            }
        }
    }

  }



void DrawVec3Control(const char* label, glm::vec3& value, float step = 1.0f)
{
    ImGui::Text(label);
    ImGui::SameLine();

    ImGui::PushID(label);

    if (ImGui::Button("X+")) value.x += step;
    ImGui::SameLine();
    if (ImGui::Button("X-")) value.x -= step;

    ImGui::SameLine();
    if (ImGui::Button("Y+")) value.y += step;
    ImGui::SameLine();
    if (ImGui::Button("Y-")) value.y -= step;

    ImGui::SameLine();
    if (ImGui::Button("Z+")) value.z += step;
    ImGui::SameLine();
    if (ImGui::Button("Z-")) value.z -= step;

    ImGui::PopID();
}

void Application::ProcessVoiceCommand(const std::wstring& cmd)
{

    if (selectedEntity < 0 || selectedEntity >= scene.entities.size())
        return; // nadie seleccionado

    auto& e = scene.entities[selectedEntity];

    if (cmd.find(L"rotate x") != std::wstring::npos)
        scene.entities[selectedEntity].rotation.x += 10.0f;

    if (cmd.find(L"rotate y") != std::wstring::npos)
        scene.entities[selectedEntity].rotation.y += 10.0f;

    if (cmd.find(L"move forward") != std::wstring::npos)
        scene.entities[selectedEntity].position.z -= 1.0f;

    if (cmd.find(L"scale up") != std::wstring::npos)
        scene.entities[selectedEntity].scale *= 1.1f;
}



// -----------------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------------
void Application::Render()
{
    


    if (backend == RenderBackend::DirectX12)
    {
        rendererDX12->BeginFrame();
        rendererDX12->EndFrame();
        return;
    }

    // === RENDER MINIMAL DEL SUELO ===
   
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1280, 720);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  

    


    


    

    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = camera->GetProjectionMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, floorPosition);
    model = glm::rotate(model, glm::radians(floorRotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(floorRotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(floorRotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, floorScale);   

    glm::mat4 mvp = projection * view * model;




    // --- GRID ESTILO UNREAL ---
    gridShader->Bind();
    gridShader->SetMat4("u_MVP", mvp);

    // Parámetros del grid
    gridShader->SetVec3("u_GridColor", glm::vec3(0.8f, 0.8f, 0.8f));   // líneas grandes
    gridShader->SetVec3("u_SubColor", glm::vec3(0.4f, 0.4f, 0.4f));    // subdivisiones
    gridShader->SetFloat("u_GridScale", 1.0f);                         // tamaño celda
    gridShader->SetFloat("u_SubScale", 10.0f);                         // subdivisiones
    gridShader->SetFloat("u_LineWidth", 0.02f);                        // grosor
    gridShader->SetFloat("u_FadeDistance", 50.0f);                     // fade
    gridShader->SetFloat("u_CameraDistance", glm::length(camera->Position));

    // Wireframe OFF (este grid es shader, no wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  
    glBindVertexArray(simplePlaneVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    // Volver a modo sólido para el triángulo
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   // === DIBUJAR TODAS LAS ENTIDADES DE LA ESCENA ===
    for (auto& e : scene.entities)
    {
        simpleShader->Bind();

        simpleShader->SetVec3("u_Color", e.material->albedo);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, e.position);
        model = glm::rotate(model, glm::radians(e.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(e.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(e.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, e.scale);

        glm::mat4 mvp = projection * view * model;



        lightingShader.Bind();
        lightingShader.SetMat4("u_MVP", mvp);
        lightingShader.SetMat4("u_Model", model);
        lightingShader.SetVec3("u_LightPos", lightPos);
        lightingShader.SetVec3("u_ViewPos", camera->Position);

        if (e.material->albedoMap == nullptr && e.material->normalMap == nullptr)
        {
            // === ENTIDAD SIN TEXTURA ===
            simpleShader->Bind();
            simpleShader->SetVec3("u_Color", e.material->albedo);
            simpleShader->SetMat4("u_Model", model);
            simpleShader->SetMat4("u_View", camera->GetViewMatrix());
            simpleShader->SetMat4("u_Projection", camera->GetProjectionMatrix());

            glBindVertexArray(e.mesh->vao);
            glDrawElements(GL_TRIANGLES, e.mesh->indexCount, GL_UNSIGNED_INT, 0);
        }
        else
        {
            // === ENTIDAD CON TEXTURA ===
            lightingShader.Bind();
            lightingShader.SetMat4("u_Model", model);
            lightingShader.SetMat4("u_View", camera->GetViewMatrix());
            lightingShader.SetMat4("u_Projection", camera->GetProjectionMatrix());

            if (e.material->albedoMap)
                e.material->albedoMap->Bind(0);

            if (e.material->normalMap)
                e.material->normalMap->Bind(1);

            glBindVertexArray(e.mesh->vao);
            glDrawElements(GL_TRIANGLES, e.mesh->indexCount, GL_UNSIGNED_INT, 0);
         }
    }

    // === MENÚ DEL EDITOR ===
    // === IMGUI FRAME ===
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();





    // === EDITOR ===
    ImGui::Begin("Editor");
    ImGui::Text("Hi, esto es tu editor funcionando");
    ImGui::Separator();

    // ===============================
    // COLOR PANEL 
    // ===============================
    ImGui::Separator();
    ImGui::Text("Colors:");
    ImGui::Spacing();

    // Solo si hay entidad seleccionada
    if (selectedEntity >= 0 && selectedEntity < scene.entities.size())
    {
        Entity& e = scene.entities[selectedEntity];

        // Usamos directamente el color del material
        glm::vec3& color = e.material->albedo;

        ImGui::ColorEdit3("Material Color", (float*)&color);

        ImGui::Spacing();
        ImGui::Text("Quick Colors:");

        ImGui::Columns(6, nullptr, false);

        if (ImGui::ColorButton("##Red", ImVec4(1, 0, 0, 1)))  color = glm::vec3(1, 0, 0);
        ImGui::NextColumn();
        if (ImGui::ColorButton("##Green", ImVec4(0, 1, 0, 1))) color = glm::vec3(0, 1, 0);
        ImGui::NextColumn();
        if (ImGui::ColorButton("##Blue", ImVec4(0, 0, 1, 1)))  color = glm::vec3(0, 0, 1);
        ImGui::NextColumn();
        if (ImGui::ColorButton("##Yellow", ImVec4(1, 1, 0, 1))) color = glm::vec3(1, 1, 0);
        ImGui::NextColumn();
        if (ImGui::ColorButton("##White", ImVec4(1, 1, 1, 1)))  color = glm::vec3(1, 1, 1);
        ImGui::NextColumn();
        if (ImGui::ColorButton("##Black", ImVec4(0, 0, 0, 1)))  color = glm::vec3(0, 0, 0);
        ImGui::Columns(1);

        ImGui::Spacing();
        // Ya no hace falta Apply: el material se actualiza en tiempo real
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No entity selected.");
    }


    // GRID
    lineShader.Bind();

    // COLOR BLANCO PARA LAS LÍNEAS
    lineShader.SetVec3("u_Color", glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 gridMVP = projection * view * glm::mat4(1.0f);
    lineShader.SetMat4("u_MVP", gridMVP);

    for (int i = -10; i <= 10; i++) {
        Renderer::DrawLine({ i,0.01f,-10 }, { i,0.01f,10 });
        Renderer::DrawLine({ -10,0.01f,i }, { 10,0.01f,i });

    }

    // ===============================
// RENDER DE ENTIDADES DEL EDITOR
// ===============================

    


 

    ImGui::Text("Último comando de voz:");
    ImGui::Text("%ls", lastVoiceCommand.c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%ls", lastVoiceCommand.c_str());


    static int gizmoMode = 0;

    // === LISTA DE ENTIDADES (Scene Outliner) ===
    ImGui::Text("Entities:");
    for (int i = 0; i < scene.entities.size(); i++)
    {
        char label[32];
        sprintf(label, "Entity %d", i);

        if (ImGui::Selectable(label, selectedEntity == i))
        {
            selectedEntity = i;
        }
    }

    if (selectedEntity >= 0 && selectedEntity < scene.entities.size())
    {
        Entity& e = scene.entities[selectedEntity];

        // --- Transformaciones ---
        if (ImGui::Button("Translate X +")) e.position.x += 1.0f;
        if (ImGui::Button("Translate X -")) e.position.x -= 1.0f;
        if (ImGui::Button("Translate Y +")) e.position.y += 1.0f;
        if (ImGui::Button("Translate Y -")) e.position.y -= 1.0f;
        if (ImGui::Button("Translate Z +")) e.position.z += 1.0f;
        if (ImGui::Button("Translate Z -")) e.position.z -= 1.0f;

        // --- Rotaciones ---
        if (ImGui::Button("Rotate X +")) e.rotation.x += 5.0f;
        if (ImGui::Button("Rotate X -")) e.rotation.x -= 5.0f;
        if (ImGui::Button("Rotate Y +")) e.rotation.y += 5.0f;
        if (ImGui::Button("Rotate Y -")) e.rotation.y -= 5.0f;
        if (ImGui::Button("Rotate Z +")) e.rotation.z += 5.0f;
        if (ImGui::Button("Rotate Z -")) e.rotation.z -= 5.0f;

        // --- Escala ---
        if (ImGui::Button("Scale X +")) e.scale.x += 1.0f;
        if (ImGui::Button("Scale X -")) e.scale.x -= 1.0f;
        if (ImGui::Button("Scale Y +")) e.scale.y += 1.0f;
        if (ImGui::Button("Scale Y -")) e.scale.y -= 1.0f;
        if (ImGui::Button("Scale Z +")) e.scale.z += 1.0f;
        if (ImGui::Button("Scale Z -")) e.scale.z -= 1.0f;

        ImGui::Separator();

        // --- ELIMINAR ENTIDAD ---
        if (ImGui::Button("Delete Entity"))
        {
            scene.entities.erase(scene.entities.begin() + selectedEntity);
            selectedEntity = -1;   // limpiar selección
        }
    }

    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Text("Duplicate Cube and Floor");

    if (ImGui::Button("Duplicate Entity"))
    {
        if (selectedEntity != -1 && !scene.entities.empty())
        {
            Entity& original = scene.entities[selectedEntity];

            Entity copy;
            copy.mesh = original.mesh;
            copy.material = new Material(*original.material);

            copy.position = original.position;
            copy.rotation = original.rotation;
            copy.scale = original.scale;

            copy.colliderMin = original.colliderMin;
            copy.colliderMax = original.colliderMax;

            copy.velocity = original.velocity;
            copy.radius = original.radius;
            copy.isBall = original.isBall;
            copy.elasticity = original.elasticity;
            copy.friction = original.friction;

            scene.entities.push_back(copy);
        }
    }

    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Text("Create geometry");
    if (ImGui::Button("Create Cube"))
    {
        Entity cube;
        cube.position = glm::vec3(0, 0, 0);
        cube.rotation = glm::vec3(0, 0, 0);
        cube.scale = glm::vec3(1, 1, 1);
        cube.mesh = &cubeMesh;
        cube.material = new Material(*cubeMesh.material);


        // === Collider del cubo ===
        cube.colliderMin = glm::vec3(-0.5f, -0.5f, -0.5f);
        cube.colliderMax = glm::vec3(0.5f, 0.5f, 0.5f);

        scene.AddEntity(cube);
        

    }

    if (ImGui::Button("Create Quad"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);
        e.mesh = &quadMesh;
        e.material = new Material(*quadMesh.material);


        e.colliderMin = glm::vec3(-0.5f);
        e.colliderMax = glm::vec3(0.5f);

        scene.AddEntity(e);
    }


    if (ImGui::Button("Create Triangle"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);
        e.mesh = &triangleMesh;
        e.material = new Material(*triangleMesh.material);


        e.colliderMin = glm::vec3(-0.5f);
        e.colliderMax = glm::vec3(0.5f);

        scene.AddEntity(e);
    }

    //Circle:
    if (ImGui::Button("Create Circle"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &circleMesh;
        e.material = new Material(*circleMesh.material);


        e.colliderMin = glm::vec3(-1.0f);
        e.colliderMax = glm::vec3(1.0f);

        scene.AddEntity(e);
    }

    //Pyramid:
    if (ImGui::Button("Create Pyramid"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &pyramidMesh;
        e.material = new Material(*pyramidMesh.material);


        e.colliderMin = glm::vec3(-0.5f);
        e.colliderMax = glm::vec3(0.5f);

        scene.AddEntity(e);
    }

    //SPHERE:
    if (ImGui::Button("Create Sphere"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &sphereMesh;
        e.material = new Material(*sphereMesh.material);


        e.colliderMin = glm::vec3(-1.0f);
        e.colliderMax = glm::vec3(1.0f);

        scene.AddEntity(e);
    }

    //CYLINDER:
    if (ImGui::Button("Create Cylinder"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &cylinderMesh;
        e.material = new Material(*cylinderMesh.material);


        e.colliderMin = glm::vec3(-1.0f);
        e.colliderMax = glm::vec3(1.0f);

        scene.AddEntity(e);
    }

    //Cone:
    if (ImGui::Button("Create Cone"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &coneMesh;
        e.material = new Material(*coneMesh.material);


        e.colliderMin = glm::vec3(-1.0f);
        e.colliderMax = glm::vec3(1.0f);

        scene.AddEntity(e);
    }

    //Torus:
    if (ImGui::Button("Create Torus"))
    {
        Entity e;
        e.position = glm::vec3(0);
        e.rotation = glm::vec3(0);
        e.scale = glm::vec3(1);

        e.mesh = &torusMesh;
        e.material = new Material(*torusMesh.material);


        e.colliderMin = glm::vec3(-1.3f);
        e.colliderMax = glm::vec3(1.3f);

        scene.AddEntity(e);
    }

    std::vector<float> hsVertices;
    std::vector<uint32_t> hsIndices;

int segX = 32;
int segY = 16;

for (int y = 0; y <= segY; y++)
{
    for (int x = 0; x <= segX; x++)
    {
        float u = (float)x / segX;
        float v = (float)y / segY;

        float theta = u * 2.0f * 3.14159f;
        float phi   = v * 3.14159f * 0.5f; // solo media esfera

        float nx = cos(theta) * sin(phi);
        float ny = cos(phi);
        float nz = sin(theta) * sin(phi);

        float px = nx;
        float py = ny;
        float pz = nz;

        hsVertices.insert(hsVertices.end(), {px,py,pz, nx,ny,nz, u,v});
    }
}


if (ImGui::Button("Create Tube"))
{
    Entity e;
    e.position = glm::vec3(0.0f);
    e.rotation = glm::vec3(0.0f);
    e.scale = glm::vec3(1.0f);

    e.mesh = &tubeMesh;
    e.material = new Material(*tubeMesh.material);


    scene.AddEntity(e);
}


if (ImGui::Button("Create Capsule"))
{
    Entity e;
    e.position = glm::vec3(0.0f);
    e.rotation = glm::vec3(0.0f);
    e.scale = glm::vec3(1.0f);

    e.mesh = &capsuleMesh;
    e.material = new Material(*capsuleMesh.material);


    scene.AddEntity(e);
}

if (ImGui::Button("Create DoorFrame"))
{
    Entity e;
    e.position = glm::vec3(0.0f);
    e.rotation = glm::vec3(0.0f);
    e.scale = glm::vec3(1.0f);

    e.mesh = &doorFrameMesh;
    e.material = new Material(*doorFrameMesh.material);


    scene.AddEntity(e);
}


if (ImGui::Button("Create Door"))
{
    Entity e;
    e.position = glm::vec3(0.0f);
    e.rotation = glm::vec3(0.0f);
    e.scale = glm::vec3(1.0f);

    e.mesh = &doorMesh;
    e.material = new Material(*doorMesh.material);


    scene.AddEntity(e);
}


if (ImGui::Button("Create Bouncing Ball"))
{
    Entity e;
    e.position = glm::vec3(0, 5, 0);
    e.rotation = glm::vec3(0);
    e.scale = glm::vec3(1);

    e.mesh = &sphereMesh;
    e.material = new Material(*sphereMesh.material);


    e.velocity = glm::vec3(0);
    e.radius = 0.5f;
    e.isBall = true;

    scene.AddEntity(e);
}


if (ImGui::Button("Create Ladder"))
{
    Entity e;
    e.position = glm::vec3(0, 0, 0);
    e.rotation = glm::vec3(0);
    e.scale = glm::vec3(1);

    e.mesh = &ladderMesh;
    e.material = new Material(*ladderMesh.material);


    scene.AddEntity(e);
}

if (ImGui::Button("Create Modern Staircase"))
{
    Entity e;
    e.position = glm::vec3(0, 0, 0);
    e.rotation = glm::vec3(0);
    e.scale = glm::vec3(1);

    e.mesh = &modernStairMesh;
    e.material = new Material(*modernStairMesh.material);


    scene.AddEntity(e);
}


if (ImGui::Button("Create Table"))
{
    Entity e;
    e.position = glm::vec3(0);
    e.rotation = glm::vec3(0);
    e.scale = glm::vec3(1);

    e.mesh = &tableMesh;
    e.material = new Material(*tableMesh.material);

    scene.AddEntity(e);
}


if (ImGui::Button("Create stick"))
{
    Entity e;
    e.position = glm::vec3(0);
    e.rotation = glm::vec3(0);
    e.scale = glm::vec3(1);

    e.mesh = &knifeMesh;
    e.material = new Material(*knifeMesh.material);

    scene.AddEntity(e);
}

if (ImGui::Button("Crear Drone"))
{
    Entity drone;
    drone.position = glm::vec3(0, 0, 0);
    drone.rotation = glm::vec3(0);
    drone.scale = glm::vec3(1);

    drone.mesh = &droneMesh;
    drone.material = new Material();


    scene.AddEntity(drone);
}



ImGui::Begin("Scene Controls");

if (!isPlaying)
{
    if (ImGui::Button("Play"))
    {
        isPlaying = true;
        activeCamera = playerCamera;
        // Activar cámara del jugador
        camera->Position = glm::vec3(1.0f, 1.0f, 1.0f); // posición inicial
        camera->Yaw = 90.0f;
        camera->Pitch = 0.0f;

        // Bloquear el ratón como en un juego FPS
       // Bloquear el ratón como en un FPS
        glfwSetInputMode(Application::window->GetNativeWindow(),
            GLFW_CURSOR,
            GLFW_CURSOR_DISABLED);

        int axesCount;
        const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axesCount);
        
    }
}
else
{
    if (ImGui::Button("Stop"))
    {
        isPlaying = false;
        activeCamera = editorCamera;
        // Liberar el ratón para volver al editor
        glfwSetInputMode(Application::window->GetNativeWindow(),
            GLFW_CURSOR,
            GLFW_CURSOR_NORMAL);
    }
}

ImGui::End();







ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Separator();

ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Separator();
ImGui::Text("Floor:Translate,Rotate,Reset,Scale");

//FloorTranslate
    if (ImGui::Button("TranslateFloor"))
    {

        floorPosition.x += 1.0f;

    }

    if (ImGui::Button("BackToFloor"))
    {

        floorPosition.x -= 1.0f;

    }


    if (ImGui::Button("TranslateFloorY"))
    {

        floorPosition.y += 1.0f;

    }

    if (ImGui::Button("BackTranslateFloorY"))
    {

        floorPosition.y -= 1.0f;

    }

    if (ImGui::Button("TranslateFloorZ"))
    {

        floorPosition.z += 1.0f;

    }

    if (ImGui::Button("BackTranslateFloorZ"))
    {

        floorPosition.z -= 1.0f;

    }

    //ResetFloor
    if (ImGui::Button("ResetFloor"))
    {
        floorPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        floorRotation = glm::vec3(0.0f);
        floorScale = glm::vec3(1.0f);
    }

    //FloorRotate
    if (ImGui::Button("FloorRotateXRight"))
    {

        floorRotation.x += 5.0f;

    }

    if (ImGui::Button("FloorRotateXLeft"))
    {

        floorRotation.x -= 5.0f;

    }

    if (ImGui::Button("FloorRotateYRight"))
    {

        floorRotation.y += 5.0f;

    }

    if (ImGui::Button("FloorRotateYLeft"))
    {

        floorRotation.y -= 5.0f;

    }

    if (ImGui::Button("FloorRotateZRight"))
    {

        floorRotation.z += 5.0f;

    }

    if (ImGui::Button("FloorRotateZLeft"))
    {

        floorRotation.z -= 5.0f;

    }

    //ScaleFloor:
    if (ImGui::Button("ScaleFloorX"))
    {

        floorScale.x += 1.0f;


    }

    if (ImGui::Button("BackScaleFloorX"))
    {

        floorScale.x -= 1.0f;


    }


    if (ImGui::Button("ScaleFloorY"))
    {

        floorScale.y += 7.0f;


    }

    if (ImGui::Button("BackScaleFloorY"))
    {

        floorScale.y -= 7.0f;


    }

    if (ImGui::Button("ScaleFloorZ"))
    {

        floorScale.z += 1.0f;


    }

    if (ImGui::Button("BackScaleFloorZ"))
    {

        floorScale.z -= 1.0f;


    }

    ImGui::Separator();
    ImGui::Separator();

    


//Translate Cube

    if (selectedEntity >= 0 && selectedEntity < scene.entities.size())
    {
        Entity& e = scene.entities[selectedEntity];

        if (ImGui::Button("Translate X +"))
            e.position.x += 1.0f;

        if (ImGui::Button("Translate X -"))
            e.position.x -= 1.0f;

        if (ImGui::Button("Translate Y +"))
            e.position.y += 1.0f;

        if (ImGui::Button("Translate Y -"))
            e.position.y -= 1.0f;

        if (ImGui::Button("Translate Z +"))
            e.position.z += 1.0f;

        if (ImGui::Button("Translate Z -"))
            e.position.z -= 1.0f;

        if (ImGui::Button("Rotate X +"))
            e.rotation.x += 5.0f;

        if (ImGui::Button("Rotate X -"))
            e.rotation.x -= 5.0f;

        if (ImGui::Button("Rotate Y +"))
            e.rotation.y += 5.0f;

        if (ImGui::Button("Rotate Y -"))
            e.rotation.y -= 5.0f;

        if (ImGui::Button("Rotate Z +"))
            e.rotation.z += 5.0f;

        if (ImGui::Button("Rotate Z -"))
            e.rotation.z -= 5.0f;

        if (ImGui::Button("Scale X +"))
            e.scale.x += 1.0f;

        if (ImGui::Button("Scale X -"))
            e.scale.x -= 1.0f;

        if (ImGui::Button("Scale Y +"))
            e.scale.y += 1.0f;

        if (ImGui::Button("Scale Y -"))
            e.scale.y -= 1.0f;

        if (ImGui::Button("Scale Z +"))
            e.scale.z += 1.0f;

        if (ImGui::Button("Scale Z -"))
            e.scale.z -= 1.0f;
    }



    ImGui::End();

    // === GIZMO ===
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    // Posición y tamaño del gizmo (ocupa toda la ventana)
    ImGuizmo::SetRect(0, 0, 1280, 720);
    // Selección del modo
    ImGuizmo::OPERATION op;
    if (gizmoMode == 0) op = ImGuizmo::TRANSLATE;
    if (gizmoMode == 1) op = ImGuizmo::ROTATE;
    if (gizmoMode == 2) op = ImGuizmo::SCALE;

   // MATRIZ DEL SUELO
  // === GIZMO PARA ENTIDAD SELECCIONADA ===
    if (selectedEntity >= 0 && selectedEntity < scene.entities.size())
    {
        Entity& e = scene.entities[selectedEntity];

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, e.position);
        model = glm::rotate(model, glm::radians(e.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(e.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(e.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, e.scale);

        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            op,
            ImGuizmo::LOCAL,
            glm::value_ptr(model)
        );

        if (ImGuizmo::IsUsing())
        {
            float t[3], r[3], s[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), t, r, s);

            e.position = glm::vec3(t[0], t[1], t[2]);
            e.rotation = glm::vec3(r[0], r[1], r[2]);
            e.scale = glm::vec3(s[0], s[1], s[2]);
        }
    }

    // === RENDER IMGUI ===
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}
Application::~Application()
{
    if (backend == RenderBackend::OpenGL)
    {
        // Buffers de geometría
        if (cubeVBO) { delete cubeVBO; cubeVBO = nullptr; }
        if (cubeIBO) { delete cubeIBO; cubeIBO = nullptr; }
        if (planeVBO) { delete planeVBO; planeVBO = nullptr; }
        if (planeIBO) { delete planeIBO; planeIBO = nullptr; }

        if (cubeVAO) { delete cubeVAO; cubeVAO = nullptr; }
        if (planeVAO) { delete planeVAO; planeVAO = nullptr; }

        // Quad fullscreen
        if (quadVBO != 0)
        {
            glDeleteBuffers(1, &quadVBO);
            quadVBO = 0;
        }
        if (quadVAO != 0)
        {
            glDeleteVertexArrays(1, &quadVAO);
            quadVAO = 0;
        }

        // Shadow map
        if (depthMapFBO != 0)
        {
            glDeleteFramebuffers(1, &depthMapFBO);
            depthMapFBO = 0;
        }
        if (depthMap != 0)
        {
            glDeleteTextures(1, &depthMap);
            depthMap = 0;
        }

        // Final FBO
        if (finalFBO != 0)
        {
            glDeleteFramebuffers(1, &finalFBO);
            finalFBO = 0;
        }
        if (finalColor != 0)
        {
            glDeleteTextures(1, &finalColor);
            finalColor = 0;
        }
        if (finalDepth != 0)
        {
            glDeleteTextures(1, &finalDepth);
            finalDepth = 0;
        }

        // Ruido
        if (noiseTextureID != 0)
        {
            glDeleteTextures(1, &noiseTextureID);
            noiseTextureID = 0;
        }

        // Bloom
        if (bloomFBO != 0)
        {
            glDeleteFramebuffers(1, &bloomFBO);
            bloomFBO = 0;
        }
        if (bloomColor != 0)
        {
            glDeleteTextures(1, &bloomColor);
            bloomColor = 0;
        }

        // Ping-pong
        glDeleteFramebuffers(2, pingpongFBO);
        glDeleteTextures(2, pingpongColor);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    if (rendererDX12)
    {
        delete rendererDX12;
        rendererDX12 = nullptr;
    }

    if (camera)
    {
        delete camera;
        camera = nullptr;
    }

    if (window)
    {
        delete window;
        window = nullptr;
    }
}

