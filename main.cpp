#include <iostream>
#include <cstdio>
#include <cstring>
#include <GL/glew.h>

#include "Automaton.h"
#include "GLFW/glfw3.h"
#include "shader.hpp"
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>

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

GLuint programID;

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

GLint uni_nbIter;
uint32_t nbIteration = 0;
uint32_t nbInstance = 1;

automaton::Automaton automate;

// ***** Info GPU ***** //

GLuint ssbo_state, ssbo_transform, ssbo_code;

struct StateData {
    uint32_t nbTransform;
    uint32_t padding;
};

struct TransformData {
    glm::mat4 mat;
    alignas(16) uint32_t nextState;
};

//Data GPU formated
std::vector<StateData> states;
std::vector<TransformData> transforms;
std::vector<float> codes;

void updateAutomate();
void generateGPUData();
void updateSSBO();

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
                nbIteration = nbIteration+1;
                updateAutomate();
                break;
            case GLFW_KEY_KP_SUBTRACT:
                if(nbIteration > 0)
                {
                    nbIteration = nbIteration-1;
                    updateAutomate();
                }
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

    programID = LoadShaders("shaders/ifs_gpu.vert", "shaders/basic.frag");

    uni_MVP = glGetUniformLocation(programID, "MVP");
    uni_view = glGetUniformLocation(programID, "VIEW");
    uni_model = glGetUniformLocation(programID, "MODEL");
    uni_perspective = glGetUniformLocation(programID, "PERSPECTIVE");
    uni_nbIter = glGetUniformLocation(programID, "nbIteration");

}

int main(int argc, char **argv) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Spécification des attributs pour le contexte OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Profil core (sans fonctions obsolètes)

    // Création de la fenêtre
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "Ombrage", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Définir le contexte OpenGL actuel
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
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
    std::cout << "UBO Max Size : " << maxUBOSize << std::endl << std::endl;

    Projection = glm::perspective(glm::radians(60.f), 1.0f, 0.1f, 1000.0f);

    //---------Init Automaton---------//

    //state Sierpiński
    automaton::State S;

    const automaton::Transition S1(
            glm::mat4(0.5, 0, 0, 0,
                              0, 0.5, 0, 0,
                              0, 0, 1, 0,
                              0, 0, 0, 1),
            0             //Next state
    );

    const automaton::Transition S2(
            glm::mat4(0.5, 0, 0,0,
                              0, 0.5, 0, 0,
                              0, 0, 1, 0,
                              1.0/4, 0.5, 0, 1),
            0
    );

    const automaton::Transition S3(
            glm::mat4(0.5, 0, 0,0.0,
                              0, 0.5, 0, 0.0,
                              0, 0, 1, 0,
                              .5, 0, 0, 1),
            0
    );

    const automaton::Transition S4(
            glm::mat4(1, 0, 0,0.0,
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
                  1.0/4, 0.5, -.8, 1);

    const automaton::Transition C1(
            glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
            1
    );

    tmp = (1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, -.2, 1, 0,
                        1.0/4, 0.5, .8, 1);

    const automaton::Transition C2(
            glm::rotate(tmp, float(M_PIl) / 8.0f, glm::vec3(0, 1.0f, 0)),
            1
    );

    C.addTransition(C1);
    C.addTransition(C2);

    automate.addState(C);

    //---------End init Automaton---------//

    initOpenGL();

    genereVBO();

    updateAutomate();

    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, clavier);
    glfwSetMouseButtonCallback(window, mouse);
    glfwSetCursorPosCallback(window, mouseMotion);


    while (!glfwWindowShouldClose(window)) {
        affichage();
        glfwPollEvents();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(programID);
    deleteVBO();
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

void generateGPUData()
{
    states.clear();
    transforms.clear();

    uint32_t currentPadding = 0;

    auto automateStates = automate.getStates();

    for(int i = 0; i < automateStates.size(); i++)
    {
        states.emplace_back(automateStates[i].getNumberTransform(), currentPadding);
        auto stateTransition = automateStates[i].getTransitions();

        for(int j = 0; j < stateTransition.size(); j++)
        {
            transforms.emplace_back(stateTransition[j].getTransform(), stateTransition[j].getNextState());
        }

        currentPadding += stateTransition.size();

    }
}

void updateAutomate()
{
    codes.clear();
    codes = automate.encode(nbIteration);
    nbInstance = codes.size();

    generateGPUData();
    updateSSBO();
}

void updateSSBO() {

    // SSBO StateInfo (binding = 0)
    if(glIsBuffer(ssbo_state))
        glDeleteBuffers(1, &ssbo_state);

    glGenBuffers(1, &ssbo_state);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_state);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(StateData) * states.size(), states.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_state);  // Binding = 0

    // SSBO TransformInfo (binding = 1)
    glGenBuffers(1, &ssbo_transform);
    if(glIsBuffer(ssbo_transform))
        glDeleteBuffers(1, &ssbo_transform);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_transform);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TransformData) * transforms.size(), transforms.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_transform);  // Binding = 1

    // SSBO CodeInfo (binding = 2)
    glGenBuffers(1, &ssbo_code);
    if(glIsBuffer(ssbo_code))
        glDeleteBuffers(1, &ssbo_code);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_code);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * codes.size(), codes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_code);  // Binding = 2

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
    glPointSize(2.0);

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

    glUseProgram(programID);

    // Mise à jour des matrices et de la caméra
    glUniformMatrix4fv(uni_MVP, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(uni_view, 1, GL_FALSE, &View[0][0]);
    glUniformMatrix4fv(uni_model, 1, GL_FALSE, &Model[0][0]);
    glUniformMatrix4fv(uni_perspective, 1, GL_FALSE, &Projection[0][0]);
    glUniform1ui(uni_nbIter, nbIteration);

    //Bind les SSBO

    // Dessiner les objets avec instanciation
    glViewport(0, 0, screenWidth, screenHeight);
    glBindVertexArray(VAO);  // VAO de la primitive
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr, nbInstance);  // Dessiner avec instanciation
    glBindVertexArray(0);

    glUseProgram(0);
}
