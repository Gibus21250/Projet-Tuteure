#include <iostream>
#include <cstdio>
#include <GL/glew.h>
#include <fstream>

#include "Automaton.h"
#include "GLFW/glfw3.h"
#include "shader.hpp"
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define M_PIl          3.141592653589793238462643383279502884L

int screenWidth = 800;
int screenHeight = 800;

void genereVBO();
void deleteVBO();
void traceObjet();
void affichage();
void showInfo();

// variables globales pour OpenGL
bool mouseLeftDown;
bool mouseRightDown;
bool mouseMiddleDown;
int mouseX = 0, mouseY = 0;
float cameraAngleX = 0;
float cameraAngleY = 0;
float cameraDistance = 0.;

GLuint renderProgram;

// ***** CAMERA ***** //

glm::vec3 cameraPosition(0., 0, 3.);
GLint locCameraPosition;

glm::mat4 MVP;
glm::mat4 Model, View, Projection;
GLint uni_MVP, uni_view, uni_model, uni_perspective;

// ***** PRIMITIVE ***** //

glm::vec3 primitive[3] = {
    {0, 0, 0},
    {.5, 1, 0},
    {1, 0, 0}
};

GLuint indices[3] = {0, 1, 2};

GLuint VBO_primitive, VBO_indices, VAO;

GLuint indexVertex = 0;

// ***** IFS ***** //

GLuint computeProgram;

GLint uni_nbIter;
uint32_t nbIteration = 15;
uint32_t nbInstance = 1;

automaton::Automaton automate;

// ***** Info GPU ***** //

GLuint ssbo_state, ssbo_transition, ssbo_code;
GLuint ssbo_transform;
GLint uni_compNbInstance, uni_compNbIteration;

struct StateData {
    uint32_t nbTransform;
    uint32_t padding;

    StateData(uint32_t nb, uint32_t& pad)
            : nbTransform(nb), padding(pad) {}
};

struct TransitionData {
    glm::mat4 mat;
    alignas(16) uint32_t nextState;

    TransitionData(const glm::mat4& m, uint32_t next)
            : mat(m), nextState(next) {}
};

//Data GPU formated
std::vector<StateData> states;
std::vector<TransitionData> transitions;
std::vector<float> codes;

/*
 * Send a formated automate to the GPU
 */
void sendAutomatonGPU();

/*
 * Encode each path, and send it to the GPU.
 */
void encodeAutomaton();

/*
 * Decode the automaton it to the GPU.
 */
void decodeAutomaton();

void reshape(GLFWwindow *window, int w, int h) {
    screenHeight = h;
    screenWidth = w;
    glViewport(0, 0, w, h);
    // set perspective viewing frustum
    Projection = glm::perspective(glm::radians(60.0f), (float) w / (float) h, 1.0f, 1000.0f);
}

void clavier(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        // Seulement si la touche est pressée ou répétée
        switch (key) {
            case GLFW_KEY_F:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case GLFW_KEY_E:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
            case GLFW_KEY_V:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                break;
            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_P:
                nbIteration = nbIteration+1;
                {
                    std::string title = "Automaton - Iterations: " + std::to_string(nbIteration);
                    glfwSetWindowTitle(window, title.c_str());
                }
                encodeAutomaton();
                decodeAutomaton();
                break;
            case GLFW_KEY_KP_SUBTRACT:
            case GLFW_KEY_M:
                if(nbIteration > 0)
                {
                    nbIteration = nbIteration-1;
                    encodeAutomaton();
                    decodeAutomaton();
                    {
                        std::string title = "Automaton - Iterations: " + std::to_string(nbIteration);
                        glfwSetWindowTitle(window, title.c_str());
                    }
                }
                break;
            default:
                break;
        }
    }
}

void mouse(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseLeftDown = true;
        } else if (action == GLFW_RELEASE) {
            mouseLeftDown = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            mouseRightDown = true;
        } else if (action == GLFW_RELEASE) {
            mouseRightDown = false;
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            mouseMiddleDown = true;
        } else if (action == GLFW_RELEASE) {
            mouseMiddleDown = false;
        }
    }
}

void mouseMotion(GLFWwindow *window, const double x, const double y) {
    int xPos = static_cast<int>(x);
    int yPos = static_cast<int>(y);

    if (mouseLeftDown) {
        cameraAngleY += static_cast<float>(xPos - mouseX);
        cameraAngleX += static_cast<float>(yPos - mouseY);
        mouseX = xPos;
        mouseY = yPos;
    }
    if (mouseRightDown) {
        cameraDistance += (yPos - mouseY) * 0.2f;
        mouseY = yPos;
    }
}

void initOpenGL() {
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    renderProgram = LoadShaders("../shaders/basic.vert", "../shaders/basic.frag");

    uni_MVP = glGetUniformLocation(renderProgram, "MVP");
    uni_view = glGetUniformLocation(renderProgram, "VIEW");
    uni_model = glGetUniformLocation(renderProgram, "MODEL");
    uni_perspective = glGetUniformLocation(renderProgram, "PERSPECTIVE");
    uni_nbIter = glGetUniformLocation(renderProgram, "nbIteration");


    //Loading compute shader
    std::ifstream file("../shaders/decode-ifs.comp", std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Erreur : Impossible d'ouvrir le fichier " << "shaders/decode-ifs.comp" << std::endl;
        exit(0);
    }

    std::string shaderCode;
    std::string line;
    while (getline(file, line)) {
        shaderCode += line + "\n";
    }

    file.close();

    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* shaderSource = shaderCode.c_str();
    glShaderSource(computeShader, 1, &shaderSource, nullptr);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
        std::cerr << "Erreur de compilation du Compute Shader:\n" << infoLog << std::endl;
        exit(0);
    }

    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);

    glGetProgramiv(computeProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(computeProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur de linkage du programme:\n" << infoLog << std::endl;
        exit(0);
    }

    glDeleteShader(computeShader);

    uni_compNbIteration = glGetUniformLocation(computeProgram, "nbIteration");
    uni_compNbInstance = glGetUniformLocation(computeProgram, "nbInstances");

}

void benchCPU()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(-1);
    }

    // Spécification des attributs pour le contexte OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Profil core (sans fonctions obsolètes)


    std::string title = "Automaton - Iterations: " + std::to_string(nbIteration);

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    // Définir le contexte OpenGL actuel
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(-1);
    }

    // Spécification des attributs pour le contexte OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::cout << "***** Info GPU *****" << std::endl;
    std::cout << "Fabricant : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Carte graphique: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version : " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Version GLSL : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    GLint maxUBOSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
    std::cout << "UBO Max Size : " << maxUBOSize << " octets " << std::endl;
    GLint maxSSBOSize;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);

    std::cout << "SSBO Max Size: " << maxSSBOSize << " octets" << std::endl << std::endl;

    Projection = glm::perspective(glm::radians(60.f), 1.0f, 0.1f, 1000.0f);

    //---------Init Automaton---------//

    {
        automaton::State S;

        const automaton::Transition S1(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1),
                0             //Next state
        );

        const automaton::Transition S2(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          1.0 / 4, 0.5, 0, 1),
                0
        );

        const automaton::Transition S3(
                glm::mat4(0.5, 0, 0, 0.0,
                          0, 0.5, 0, 0.0,
                          0, 0, 1, 0,
                          .5, 0, 0, 1),
                0
        );

        const automaton::Transition S4(
                glm::mat4(1, 0, 0, 0.0,
                          0, 1, 0, 0.0,
                          0, 0, 1, 0,
                          1, 0, 0, 1),
                1
        );

        S.addTransition(S1);
        S.addTransition(S2);
        S.addTransition(S3);
        S.addTransition(S4);

        automate.addState(S);

        automaton::State C;

        glm::mat4 tmp(1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, -.2, 1, 0,
                      1.0 / 4, 0.5, -.8, 1);

        const automaton::Transition C1(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                1
        );

        tmp = (1, 0, 0, 0,
                0, 1, 0, 0,
                0, -.2, 1, 0,
                1.0 / 4, 0.5, .8, 1);

        const automaton::Transition C2(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                0
        );

        C.addTransition(C1);
        C.addTransition(C2);

        automate.addState(C);
    }

    //---------End init Automaton---------//

    initOpenGL();
    genereVBO();

    sendAutomatonGPU();

    std::string filename = "cpuEncoding.csv";

    // Ouvrir un fichier en mode écriture
    std::ofstream outFile(filename);

    // Vérifier si le fichier est ouvert correctement
    if (!outFile) {
        std::cerr << "Erreur lors de l'ouverture du fichier " << filename << std::endl;
        exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start;
    std::chrono::duration<double, std::nano> duration{};
    // Écrire les entêtes dans le fichier CSV
    outFile << "NombreFeuille,float1,double1,float2,double2\n";

    for (int i = 0; i < 17; ++i)
    {

        long long nombreFeuille, tpsFloat1, tpsDouble1, tpsFloat2, tpsDouble2;

        //Encode 1
        start = std::chrono::high_resolution_clock::now();
        codes = automate.encode<float>(i);
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        nombreFeuille = codes.size();

        tpsFloat1 = duration.count();

        start = std::chrono::high_resolution_clock::now();
        auto code64 = automate.encode<double>(i);
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        tpsDouble1 = duration.count();

        //Encode 2
        start = std::chrono::high_resolution_clock::now();
        codes = automate.encode2<float>(i);
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        tpsFloat2 = duration.count();

        start = std::chrono::high_resolution_clock::now();
        auto code642 = automate.encode2<double>(i);
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        tpsDouble2 = duration.count();

        outFile << nombreFeuille << "," << tpsFloat1 << "," << tpsDouble1 << "," << tpsFloat2 << "," << tpsDouble2 << "\n";

    }

    outFile.close();

    //Init automaton on the GPU

    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, clavier);
    glfwSetMouseButtonCallback(window, mouse);
    glfwSetCursorPosCallback(window, mouseMotion);

    glDeleteProgram(renderProgram);
    deleteVBO();
}

void benchGPU()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        exit(-1);
    }

    // Spécification des attributs pour le contexte OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Profil core (sans fonctions obsolètes)


    std::string title = "Automaton - Iterations: " + std::to_string(nbIteration);

    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    // Définir le contexte OpenGL actuel
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(-1);
    }

    // Spécification des attributs pour le contexte OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::cout << "***** Info GPU *****" << std::endl;
    std::cout << "Fabricant : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Carte graphique: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version : " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Version GLSL : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    GLint maxUBOSize;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
    std::cout << "UBO Max Size : " << maxUBOSize << " octets " << std::endl;
    GLint maxSSBOSize;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);

    std::cout << "SSBO Max Size: " << maxSSBOSize << " octets" << std::endl << std::endl;

    Projection = glm::perspective(glm::radians(60.f), 1.0f, 0.1f, 1000.0f);

    //---------Init Automaton---------//

    {
        automaton::State S;

        const automaton::Transition S1(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1),
                0             //Next state
        );

        const automaton::Transition S2(
                glm::mat4(0.5, 0, 0, 0,
                          0, 0.5, 0, 0,
                          0, 0, 1, 0,
                          1.0 / 4, 0.5, 0, 1),
                0
        );

        const automaton::Transition S3(
                glm::mat4(0.5, 0, 0, 0.0,
                          0, 0.5, 0, 0.0,
                          0, 0, 1, 0,
                          .5, 0, 0, 1),
                0
        );

        const automaton::Transition S4(
                glm::mat4(1, 0, 0, 0.0,
                          0, 1, 0, 0.0,
                          0, 0, 1, 0,
                          1, 0, 0, 1),
                1
        );

        S.addTransition(S1);
        S.addTransition(S2);
        S.addTransition(S3);
        S.addTransition(S4);

        automate.addState(S);

        automaton::State C;

        glm::mat4 tmp(1, 0, 0, 0,
                      0, 1, 0, 0,
                      0, -.2, 1, 0,
                      1.0 / 4, 0.5, -.8, 1);

        const automaton::Transition C1(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                1
        );

        tmp = (1, 0, 0, 0,
                0, 1, 0, 0,
                0, -.2, 1, 0,
                1.0 / 4, 0.5, .8, 1);

        const automaton::Transition C2(
                glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
                0
        );

        C.addTransition(C1);
        C.addTransition(C2);

        automate.addState(C);
    }

    //---------End init Automaton---------//

    initOpenGL();
    genereVBO();

    GLuint computeProgram64;
    GLint uni_compNbIteration64, uni_compNbInstance64;

    //Shader 64bits
    {
        //Loading compute shader
        std::ifstream file("../shaders/decode-ifs64.comp", std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Erreur : Impossible d'ouvrir le fichier " << "shaders/decode-ifs.comp" << std::endl;
            exit(0);
        }

        std::string shaderCode;
        std::string line;
        while (getline(file, line)) {
            shaderCode += line + "\n";
        }

        file.close();

        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        const char* shaderSource = shaderCode.c_str();
        glShaderSource(computeShader, 1, &shaderSource, nullptr);
        glCompileShader(computeShader);

        GLint success;
        glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(computeShader, 512, nullptr, infoLog);
            std::cerr << "Erreur de compilation du Compute Shader:\n" << infoLog << std::endl;
            exit(0);
        }

        computeProgram64 = glCreateProgram();
        glAttachShader(computeProgram64, computeShader);
        glLinkProgram(computeProgram64);

        glGetProgramiv(computeProgram64, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(computeProgram, 512, nullptr, infoLog);
            std::cerr << "Erreur de linkage du programme:\n" << infoLog << std::endl;
            exit(0);
        }

        glDeleteShader(computeShader);

        uni_compNbIteration64 = glGetUniformLocation(computeProgram64, "nbIteration");
        uni_compNbInstance64 = glGetUniformLocation(computeProgram64, "nbInstances");

    }

    sendAutomatonGPU();

    std::string filename = "gpuDecoding.csv";

    // Ouvrir un fichier en mode écriture
    std::ofstream outFile(filename);

    // Vérifier si le fichier est ouvert correctement
    if (!outFile) {
        std::cerr << "Erreur lors de l'ouverture du fichier " << filename << std::endl;
        exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start;
    std::chrono::duration<double, std::nano> duration{};
    // Écrire les entêtes dans le fichier CSV
    outFile << "NombreFeuille,verite,cpu32,gpu32,cpu64,gpu64\n";

    GLuint ssbo_bench, ssbo_bench64;

    std::vector<float> code32;
    std::vector<double> code64;

    uint32_t nombreFeuille;

    for (int i = 0; i < 16; ++i)
    {

        long long tpsVerite, tpsCPU32, tpsGPU32, tpsCPU64, tpsGPU64;

        start = std::chrono::high_resolution_clock::now();
        auto veri = std::move(automate.compute(i));
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        tpsVerite = duration.count();


        //float
        {
            code32 = std::move(automate.encode2<float>(i));
            nombreFeuille = code32.size();

            start = std::chrono::high_resolution_clock::now();
            auto decoded = std::move(automate.decode(i, code32));
            end = std::chrono::high_resolution_clock::now();
            duration = end - start;

            tpsCPU32 = duration.count();

            //Test sur GPU32
            //Envoie des data
            // SSBO CodeInfo (binding = 3)
            if(glIsBuffer(ssbo_bench))
                glDeleteBuffers(1, &ssbo_bench);

            glGenBuffers(1, &ssbo_bench);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_bench);
            glBufferData(GL_SHADER_STORAGE_BUFFER, nombreFeuille * sizeof(float), code32.data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_bench);  // Binding = 3

            //Génération du buffer pour stocker les résultats
            {
                // SSBO transitions (binding = 0)
                if(glIsBuffer(ssbo_transform))
                    glDeleteBuffers(1, &ssbo_transform);

                size_t transformsSize = sizeof(glm::mat4) * nombreFeuille;

                glGenBuffers(1, &ssbo_transform);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transform);
                glBufferData(GL_SHADER_STORAGE_BUFFER, transformsSize, 0, GL_STATIC_DRAW);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_transform);  // Binding = 0
            }

            //Now dispatch GPU32

            GLuint query;
            glGenQueries(1, &query);

            glUseProgram(computeProgram);
            glUniform1ui(uni_compNbIteration, i);
            glUniform1ui(uni_compNbInstance, nombreFeuille);

            glBeginQuery(GL_TIME_ELAPSED, query);
            glDispatchCompute((nombreFeuille + 63) / 64, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glEndQuery(GL_TIME_ELAPSED);
            glUseProgram(0);

            GLuint64 elapsedTime = 0;
            do {
                glGetQueryObjectui64v(query, GL_QUERY_RESULT_AVAILABLE, &elapsedTime);
            } while (!elapsedTime);

            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsedTime);

            tpsGPU32 = elapsedTime;

            glDeleteQueries(1, &query);

        }

        //Double
        {
            code64 = std::move(automate.encode2<double>(i));

            start = std::chrono::high_resolution_clock::now();
            auto decoded = std::move(automate.decode(i, code64));
            end = std::chrono::high_resolution_clock::now();
            duration = end - start;

            tpsCPU64 = duration.count();

            //Test sur GPU64
            //Envoie des data
            // SSBO CodeInfo (binding = 3)
            if(glIsBuffer(ssbo_bench64))
                glDeleteBuffers(1, &ssbo_bench64);

            glGenBuffers(1, &ssbo_bench64);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_bench64);
            glBufferData(GL_SHADER_STORAGE_BUFFER, nombreFeuille * sizeof(double), code64.data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_bench64);  // Binding = 3

            //Now dispatch GPU32

            GLuint query;
            glGenQueries(1, &query);

            glUseProgram(computeProgram64);
            glUniform1ui(uni_compNbIteration64, i);
            glUniform1ui(uni_compNbInstance64, nombreFeuille);

            glBeginQuery(GL_TIME_ELAPSED, query);
            glDispatchCompute((nombreFeuille + 63) / 64, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            glEndQuery(GL_TIME_ELAPSED);
            glUseProgram(0);

            GLuint64 elapsedTime = 0;
            do {
                glGetQueryObjectui64v(query, GL_QUERY_RESULT_AVAILABLE, &elapsedTime);
            } while (!elapsedTime);

            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsedTime);

            tpsGPU64 = elapsedTime;

            glDeleteQueries(1, &query);

        }

        outFile << nombreFeuille << "," << tpsVerite <<  "," << tpsCPU32 << "," << tpsGPU32 << "," << tpsCPU64 << "," << tpsGPU64 << "\n";

    }

    outFile.close();

    //Init automaton on the GPU

    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, clavier);
    glfwSetMouseButtonCallback(window, mouse);
    glfwSetCursorPosCallback(window, mouseMotion);

    glDeleteProgram(renderProgram);
    deleteVBO();
}

int main(int argc, char **argv)
{
    benchGPU();
    return 0;
}

void genereVBO() {

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glEnableVertexAttribArray(indexVertex); // Location vertex = 0
    if (glIsBuffer(VBO_primitive) == GL_TRUE)
        glDeleteBuffers(1, &VBO_primitive);

    glGenBuffers(1, &VBO_primitive);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_primitive);
    glBufferData(GL_ARRAY_BUFFER, sizeof(primitive), primitive, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(indexVertex, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // **Indiquer qu'il s'agit d'un attribut instancié**
    // L'attribut de position ne change pas entre les instances, donc on utilise glVertexAttribDivisor
    // Cela dit à OpenGL d'utiliser la même position pour toutes les instances
    glVertexAttribDivisor(indexVertex, 0);

    if (glIsBuffer(VBO_indices) == GL_TRUE)
        glDeleteBuffers(1, &VBO_indices);

    glGenBuffers(1, &VBO_indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void sendAutomatonGPU()
{
    states.clear();
    transitions.clear();

    // ******* Update Structures for the GPU ******* //
    uint32_t currentPadding = 0;

    auto automateStates = automate.getStates();

    for(int i = 0; i < automateStates.size(); i++)
    {
        states.emplace_back(automateStates[i].getNumberTransform(), currentPadding);
        auto stateTransition = automateStates[i].getTransitions();

        for(int j = 0; j < stateTransition.size(); j++)
        {
            transitions.emplace_back(stateTransition[j].getTransform(), stateTransition[j].getNextState());
        }

        currentPadding += stateTransition.size();

    }

    // ******* Send Data to the GPU ******* //

    // SSBO StateInfo (binding = 1)
    if(glIsBuffer(ssbo_state))
        glDeleteBuffers(1, &ssbo_state);

    size_t stateInfoSize = sizeof(StateData) * states.size();

    glGenBuffers(1, &ssbo_state);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_state);
    glBufferData(GL_SHADER_STORAGE_BUFFER, stateInfoSize, states.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_state);  // Binding = 1

    std::cout << "SSBO StateInfo (binding = 1) - Size: " << stateInfoSize << " bytes." << std::endl;

    // SSBO TransitionInfo (binding = 2)
    if(glIsBuffer(ssbo_transition))
        glDeleteBuffers(1, &ssbo_transition);

    size_t transitionInfoSize = sizeof(TransitionData) * transitions.size();

    glGenBuffers(1, &ssbo_transition);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transition);
    glBufferData(GL_SHADER_STORAGE_BUFFER, transitionInfoSize, transitions.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_transition);  // Binding = 2


    std::cout << "SSBO TransitionInfo (binding = 2) - Size: " << transitionInfoSize << " bytes." << std::endl;

}

void encodeAutomaton()
{
    //Add logic if uniform automaton, no need to encode CPU side

    auto start = std::chrono::high_resolution_clock::now();
    codes = automate.encode2<float>(nbIteration);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Encoding execution timme : " << duration.count() << " ms." << std::endl;

    nbInstance = codes.size();


    // SSBO CodeInfo (binding = 3)
    if(glIsBuffer(ssbo_code))
        glDeleteBuffers(1, &ssbo_code);

    size_t codeInfoSize = sizeof(float) * codes.size();

    glGenBuffers(1, &ssbo_code);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_code);
    glBufferData(GL_SHADER_STORAGE_BUFFER, codeInfoSize, codes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_code);  // Binding = 3

    std::cout << "SSBO CodeInfo (binding = 3) - Size: " << codeInfoSize << " bytes." << std::endl;

}

void decodeAutomaton(){

    // SSBO transitions (binding = 0)
    if(glIsBuffer(ssbo_transform))
        glDeleteBuffers(1, &ssbo_transform);

    size_t transformsSize = sizeof(glm::mat4) * codes.size();

    glGenBuffers(1, &ssbo_transform);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transform);
    glBufferData(GL_SHADER_STORAGE_BUFFER, transformsSize, 0, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_transform);  // Binding = 0

    std::cout << "SSBO Transforms (binding = 0) - Size: " << transformsSize << " bytes." << std::endl;


    //Now dispatch the decoder shader
    GLuint query;
    glGenQueries(1, &query);


    glUseProgram(computeProgram);
    glUniform1ui(uni_compNbIteration, nbIteration);
    glUniform1ui(uni_compNbInstance, nbInstance);

    glBeginQuery(GL_TIME_ELAPSED, query);
    glDispatchCompute((nbInstance + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glFlush();
    glEndQuery(GL_TIME_ELAPSED);
    glUseProgram(0);

    GLuint64 elapsedTime = 0;
    do {
        glGetQueryObjectui64v(query, GL_QUERY_RESULT_AVAILABLE, &elapsedTime);
    } while (!elapsedTime);

    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsedTime);

    std::cout << "Decoding shader execution time: " << elapsedTime / 1e6 << " ms" << std::endl;

    glDeleteQueries(1, &query);

}

void deleteVBO() {
    glDeleteBuffers(1, &VBO_primitive);
    glDeleteBuffers(1, &VBO_indices);
    glDeleteBuffers(1, &VAO);
}

void affichage() {
    /* effacement de l'image avec la couleur de fond */
    /* Initialisation d'OpenGL */
    glClearColor(.8, .8, 1.0, 0.0);
    glClearDepth(10); // 0 is near, >0 is far
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Model = glm::mat4(1.0f);
    Model = translate(Model, glm::vec3(0, 0, cameraDistance));
    Model = rotate(Model, glm::radians(cameraAngleX), glm::vec3(1, 0, 0));
    Model = rotate(Model, glm::radians(cameraAngleY), glm::vec3(0, 1, 0));
    Model = scale(Model, glm::vec3(.8, .8, .8));

    View = lookAt(cameraPosition,
                       glm::vec3(0, 0, 0),
                       glm::vec3(0, 1, 0)
    );


    MVP = Projection * View * Model;

    traceObjet();
}

void traceObjet() {

    glUseProgram(renderProgram);

    // Mise à jour des matrices et de la caméra
    glUniformMatrix4fv(uni_MVP, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(uni_view, 1, GL_FALSE, &View[0][0]);
    glUniformMatrix4fv(uni_model, 1, GL_FALSE, &Model[0][0]);
    glUniformMatrix4fv(uni_perspective, 1, GL_FALSE, &Projection[0][0]);
    glUniform1ui(uni_nbIter, nbIteration);

    // Dessiner les objets avec instanciation
    glViewport(0, 0, screenWidth, screenHeight);
    glBindVertexArray(VAO);  // VAO de la primitive
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr, nbInstance);
    glBindVertexArray(0);

    glUseProgram(0);
}
