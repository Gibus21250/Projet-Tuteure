#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <GL/glew.h>

#include "GLFW/glfw3.h"
#include "shader.hpp"
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

// variables globales pour OpenGL
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
GLint MatrixIDMVP, MatrixIDView, MatrixIDModel, MatrixIDPerspective;

// ***** PRIMITIVE ***** //

glm::vec3 primitive[3] = {
    {0, -1.0, 0},
    {-1, 2, 0},
    {1, 1.5, 0}
};

GLuint indices[3] = {0, 1, 2};

GLuint VBO_primitive, VBO_indices, VAO;

GLuint indexVertex = 0;

// ***** TRANSFORMS ***** //
std::vector transforms = {

        glm::mat4(0.5, 0, 0, 0,
                  0, 0.5, 0, -0.5,
                  0, 0, 1, 0,
                  0, 0, 0, 1),

        glm::mat4(0.5, 0, 0,-0.5,
                  0, 0.5, 0,0.5,
                  0, 0, 1, 0,
                  0, 0, 0, 1),

        glm::mat4(0.5, 0, 0, 0.5,
                  0, 0.5, 0, 0.5,
                  0, 0, 1, 0,
                  0, 0, 0, 1)

};

GLuint transformsBlockID, drawInfoBlockID;

// ***** Info Dessin ***** //
struct DrawInfo
{
    unsigned nbIteration;
    unsigned maxInstance;
    unsigned nbTransformation;
    unsigned padding;
};

DrawInfo drawInfo;
GLuint uboTranforms, uboInfo;

void reshape(GLFWwindow *window, int w, int h) {
    screenHeight = h;
    screenWidth = w;
    glViewport(0, 0, w, h);
    // set perspective viewing frustum
    Projection = glm::perspective(glm::radians(60.0f), (float) w / (float) h, 1.0f, 1000.0f);
}

void clavier(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        unsigned iter;
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
                iter = drawInfo.nbIteration+1;
                updateDrawInfo(iter);
                break;
            case GLFW_KEY_KP_SUBTRACT:
                iter = drawInfo.nbIteration-1;
                if (drawInfo.nbIteration > 0)
                    updateDrawInfo(iter);
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

    programID = LoadShaders("shaders/ifs_instanced.vert", "shaders/PhongShader.frag");

    MatrixIDMVP = glGetUniformLocation(programID, "MVP");
    MatrixIDView = glGetUniformLocation(programID, "VIEW");
    MatrixIDModel = glGetUniformLocation(programID, "MODEL");
    MatrixIDPerspective = glGetUniformLocation(programID, "PERSPECTIVE");

    locCameraPosition = glGetUniformLocation(programID, "cameraPosition");

    // UBO Transforms
    glGenBuffers(1, &uboTranforms);
    glBindBuffer(GL_UNIFORM_BUFFER, uboTranforms);
    glBufferData(GL_UNIFORM_BUFFER, drawInfo.nbTransformation * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);

    transformsBlockID = glGetUniformBlockIndex(programID, "TransformsBlock");

    glUniformBlockBinding(programID, transformsBlockID, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboTranforms);

    // UBO DrawInfo

    glGenBuffers(1, &uboInfo);
    glBindBuffer(GL_UNIFORM_BUFFER, uboInfo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(DrawInfo), nullptr, GL_DYNAMIC_DRAW);

    drawInfoBlockID = glGetUniformBlockIndex(programID, "DrawInfoBlock");

    glUniformBlockBinding(programID, drawInfoBlockID, 1);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboInfo);
}

void updateDrawInfo(unsigned nbIteration) {

    if (nbIteration > drawInfo.nbIteration)
        drawInfo.maxInstance *= drawInfo.nbTransformation;
    else
        drawInfo.maxInstance /= drawInfo.nbTransformation;

    drawInfo.nbIteration = nbIteration;
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
    drawInfo.nbIteration = 1;
    drawInfo.maxInstance = 3;
    drawInfo.nbTransformation = transforms.size();

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
    glUniformMatrix4fv(MatrixIDMVP, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(MatrixIDView, 1, GL_FALSE, &View[0][0]);
    glUniformMatrix4fv(MatrixIDModel, 1, GL_FALSE, &Model[0][0]);
    glUniformMatrix4fv(MatrixIDPerspective, 1, GL_FALSE, &Projection[0][0]);
    glUniform3f(locCameraPosition, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Mettre à jour les données des UBOs après les avoir liés
    glBindBuffer(GL_UNIFORM_BUFFER, uboTranforms);
    glBufferData(GL_UNIFORM_BUFFER, drawInfo.nbTransformation * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, uboInfo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(DrawInfo), &drawInfo, GL_STATIC_DRAW);

    // Dessiner les objets avec instanciation
    glViewport(0, 0, screenWidth, screenHeight);
    glBindVertexArray(VAO);  // VAO de la primitive
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr, drawInfo.maxInstance);  // Dessiner avec instanciation
    glBindVertexArray(0);

    glUseProgram(0);
}
