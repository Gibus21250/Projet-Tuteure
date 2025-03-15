#include <iostream>
#include <cstdio>
#include <cstring>
#include <GL/glew.h>

#include "Automaton.h"
#include "GLFW/glfw3.h"
#include "shader.hpp"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"

#include "glm/gtc/matrix_transform.hpp"

int screenWidth = 800;
int screenHeight = 800;

void genereVBO();
void deleteVBO();
void traceObjet();
void affichage();
void showInfo();
void updateDrawInfo(unsigned nbIteration);

// Global var for control
bool mouseLeftDown;
bool mouseRightDown;
bool mouseMiddleDown;
int mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance = 0.;

GLuint programID;

// ***** CAMERA ***** //

glm::vec3 cameraPosition(0., 0, 3.);
GLint locCameraPosition;

glm::mat4 MVP;
glm::mat4 Model, View, Projection;
GLint uni_MVP, uni_view, uni_model, uni_perspective;

// ***** PRIMITIVE ***** //

glm::vec3 primitive[4] = {
    {0, 0, 0},
    {.5, 1, 0},
    {1, 0, 0},
    {.5, 0, 1}
};

glm::uvec3 indices[4] = {
    {0, 1, 2},
    {1, 3, 2},
    {0, 3, 1},
    {0, 2, 3}
};

GLuint VBO_primitive, VBO_indices, VAO;

GLuint indexVertex = 0;

// ***** IFS ***** //

automaton::Automaton automate;
uint32_t iteration = 0;
uint32_t nbInstance = 0;
GLint uni_nbInstance;

bool CPU = true; //Etat de la génération, CPU ou GPU

// ***** Info Dessin ***** //

GLuint ssboTranforms;

std::vector<glm::mat4> transformations;

void updateSSBOTransform() {

    if (ssboTranforms)
        glDeleteBuffers(1, &ssboTranforms);

    //Create a new SSBO
    glGenBuffers(1, &ssboTranforms);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboTranforms);
    //Copy data to SSBO
    glBufferData(GL_SHADER_STORAGE_BUFFER, transformations.size() * sizeof(glm::mat4), transformations.data(), GL_DYNAMIC_DRAW);

}

void reshape(GLFWwindow *window, int w, int h) {
    screenHeight = h;
    screenWidth = w;
    glViewport(0, 0, w, h);
    // set perspective viewing frustum
    Projection = glm::perspective(glm::radians(60.0f), (float) w / (float) h, 1.0f, 1000.0f);
}

void clavier(GLFWwindow *window, int key, int scancode, int action, int mods)        {
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
                iteration = iteration+1;
                transformations = automate.compute(iteration);
                updateSSBOTransform();
                nbInstance = transformations.size();
            break;
            case GLFW_KEY_KP_SUBTRACT:
                if (iteration > 0)
                {
                    iteration = iteration-1;
                    transformations = automate.compute(iteration);
                    updateSSBOTransform();
                    nbInstance = transformations.size();
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

    programID = LoadShaders("shaders/ifs_cpu.vert", "shaders/basic.frag");

    uni_MVP = glGetUniformLocation(programID, "MVP");
    uni_view = glGetUniformLocation(programID, "VIEW");
    uni_model = glGetUniformLocation(programID, "MODEL");
    uni_perspective = glGetUniformLocation(programID, "PERSPECTIVE");

    uni_nbInstance = glGetUniformLocation(programID, "numInstances");

    updateSSBOTransform();
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


    // Initialize the Automate //


    //state square
    automaton::State C;

    const automaton::Transition T1(
    glm::mat4(0.5, 0, 0, 0,
              0, 0.5, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1),
              0             //Next state
        );

    const automaton::Transition T2(
    glm::mat4(0.5, 0, 0,1/4.0,
                  0, 0.5, 0, 0.5,
                  0, 0, 1, 0,
                  0, 0, 0, 1),
              1
        );

    C.addTransition(T1);
    C.addTransition(T2);

    automate.addState(C);

    // state "Sierpiński"
    automaton::State S;

    const automaton::Transition S1(
    glm::mat4(0.5, 0.2, 0, 0,
              0, 0.5, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1),
              1             //Next state
        );

    const automaton::Transition S2(
    glm::mat4(1.1, 0, 0,0,
                  0.2, 1, 0, 1,
                  0, .2, 1, 0,
                  0, 0, 0, 1),
              1
        );
    const automaton::Transition S3 (
    glm::mat4(1.1, 0, 0,0,
                  0.2, 1, 0, 1,
                  0, .2, 1, 0,
                  0, 0, 0, 1),
              1
        );

    S.addTransition(S1);
    S.addTransition(S2);
    S.addTransition(S3);

    automate.addState(S);

    transformations = automate.compute(iteration);

    const int nbI = 10;

    auto encodedValues = automate.encode(nbI);

    for (int i = 0; i < encodedValues.size(); i++) {
        std::cout << encodedValues[i] << " ";
    }
    std::cout << std::endl;


    auto decodedMatrices = automate.decode(nbI, encodedValues);

    auto verite = automate.compute(nbI);

    std::cout << (((decodedMatrices == verite)==true)?"OK":"NOK") << std::endl;

    initOpenGL();

    genereVBO();

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
    glUniform1ui(uni_nbInstance, nbInstance);

    // Dessiner les objets avec instanciation
    glViewport(0, 0, screenWidth, screenHeight);
    glBindVertexArray(VAO);  // VAO de la primitive
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboTranforms); // Associer le SSBO au binding 0
    glDrawElementsInstanced(GL_TRIANGLES, sizeof(indices) / sizeof(glm::vec1), GL_UNSIGNED_INT, nullptr, transformations.size());  // Dessiner avec instanciation
    glBindVertexArray(0);

    glUseProgram(0);
}

