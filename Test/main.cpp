#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

GLFWwindow* window;
float angleX = 0.0f;
float angleY = 0.0f;
float zoom = 5.0f;
double lastX = 400.0;
double lastY = 300.0;
const float sensitivity = 0.005f; // Sensibilité de la souris

GLuint sphereVAO, sphereVBO, groundVAO, groundVBO, wallVAO, wallVBO;
GLuint cylinderVAO, cylinderVBO, cylinderEBO;
GLuint circleVAO, circleVBO;
GLuint powerGaugeVAO, powerGaugeVBO;
GLuint flagVAO, flagVBO;
GLuint shaderProgram, ballShaderProgram, circleShaderProgram, poleShaderProgram, flagShaderProgram, gaugeShaderProgram, trailShaderProgram; // Shaders

const int sectorCount = 36;
const int stackCount = 18;
const float radius = 0.6f;
const float dampingFactor = 0.8f;
const float gravity = 0.005f;
const float minBounceSpeed = 0.001f;
const float friction = 0.995f; // Friction
const int subSteps = 10; // Nombre de sous-étapes pour la simulation
const float rotationSpeedFactor = 50.0f; // Facteur de vitesse de rotation

double keyPressDuration = 0.0;
const double maxKeyPressDuration = 3.0;
const float maxImpulseStrength = 2.0f;
double lastShotTime = -2.0; // Initialise à -2 pour permettre le premier tir immédiatement
const double shotCooldown = 2.0; // Temps de récupération en secondes
double lastTime = glfwGetTime();
double endTime = 0.0;
bool showEndText = false;
int numShots = 0; // Nombre de tirs

const int maxTrailLength = 50; // Longueur maximale de la traînée
std::vector<glm::vec3> trailPositions; // Vecteur pour stocker les positions de la traînée

const double levelTransitionDelay = 0.0; // Délai avant la transition vers le niveau suivant
bool levelTransition = false; // Drapeau pour indiquer la transition de niveau

glm::vec3 initialSpherePosition(0.0f, radius, 0.0f); // Position initiale au sol
glm::vec3 spherePosition = initialSpherePosition;
glm::vec3 sphereVelocity(0.0f, 0.0f, 0.0f);
glm::mat4 ballRotation = glm::mat4(1.0f); // Matrice de rotation initiale pour la balle

bool cursorLocked = true;
int currentCourse = 0; // Variable pour suivre le parcours actuel

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;
    angleY += deltaX * sensitivity;
    angleX += deltaY * sensitivity;
    lastX = xpos;
    lastY = ypos;

    if (angleX > glm::radians(89.0f))
        angleX = glm::radians(89.0f);
    if (angleX < glm::radians(-89.0f))
        angleX = glm::radians(-89.0f);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoom -= yoffset * 0.5f;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 20.0f)
        zoom = 20.0f;
}

void setupSphere()
{
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);

    std::vector<GLfloat> sphereVertices;
    std::vector<GLfloat> sphereColors;
    std::vector<GLfloat> sphereTexCoords;
    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * glm::pi<float>() / sectorCount;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);

            sphereColors.push_back(1.0f);
            sphereColors.push_back(1.0f);
            sphereColors.push_back(1.0f);

            sphereTexCoords.push_back((float)j / sectorCount);
            sphereTexCoords.push_back((float)i / stackCount);
        }
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), sphereVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    GLuint colorBuffer;
    glGenBuffers(1, &colorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereColors.size(), sphereColors.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    GLuint texCoordBuffer;
    glGenBuffers(1, &texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereTexCoords.size(), sphereTexCoords.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

    glBindVertexArray(0);
}

void setupGround()
{
    glGenVertexArrays(1, &groundVAO);
    glBindVertexArray(groundVAO);

    glGenBuffers(1, &groundVBO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);

    // Définir les sommets du sol pour plusieurs parcours
    std::vector<std::vector<GLfloat>> groundVertices = {
        // Parcours 1
        {
            -5.5f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f,
            -5.5f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f,
            5.5f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f,
            5.5f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f,
            -5.5f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f,
            65.5f, 0.0f, 50.0f, 0.0f, 1.0f, 0.0f,
            65.5f, 0.0f, 65.0f, 0.0f, 1.0f, 0.0f,
            -5.5f, 0.0f, 65.0f, 0.0f, 1.0f, 0.0f
        },
        // Parcours 2
        {
            -10.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            10.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            40.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            40.0f, 0.0f, 45.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 45.0f, 0.0f, 1.0f, 0.0f
        },
        // Parcours 3
        {
            -3.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f,
            -3.0f, 0.0f, 95.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 0.0f, 95.0f, 0.0f, 1.0f, 0.0f,
            3.0f, 0.0f, -5.0f, 0.0f, 1.0f, 0.0f
        }
    };

    // Configuration initiale du parcours
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * groundVertices[currentCourse].size(), groundVertices[currentCourse].data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void setupWalls()
{
    glGenVertexArrays(1, &wallVAO);
    glBindVertexArray(wallVAO);

    glGenBuffers(1, &wallVBO);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO);

    // Définir les sommets des murs pour plusieurs parcours
    std::vector<std::vector<GLfloat>> wallVertices = {
        // Parcours 1
        {
            -5.5f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 2.0f, 65.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 0.0f, 65.0f, 0.5f, 0.5f, 0.5f,

            5.5f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 2.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 0.0f, 50.0f, 0.5f, 0.5f, 0.5f,

            65.5f, 0.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            65.5f, 2.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 2.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 0.0f, 50.0f, 0.5f, 0.5f, 0.5f,

            65.5f, 0.0f, 65.0f, 0.5f, 0.5f, 0.5f,
            65.5f, 2.0f, 65.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 2.0f, 65.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 0.0f, 65.0f, 0.5f, 0.5f, 0.5f,

            -5.5f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            5.5f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            -5.5f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,

            65.5f, 0.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            65.5f, 2.0f, 50.0f, 0.5f, 0.5f, 0.5f,
            65.5f, 2.0f, 65.0f, 0.5f, 0.5f, 0.5f,
            65.5f, 0.0f, 65.0f, 0.5f, 0.5f, 0.5f
        },
        // Parcours 2
        {
            -10.0f, 0.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 2.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 2.0f, 45.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 0.0f, 45.0f, 0.5f, 0.5f, 0.5f,

            10.0f, 0.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 2.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 2.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 0.0f, 30.0f, 0.5f, 0.5f, 0.5f,

            40.0f, 0.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            40.0f, 2.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 2.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 0.0f, 30.0f, 0.5f, 0.5f, 0.5f,

            40.0f, 0.0f, 45.0f, 0.5f, 0.5f, 0.5f,
            40.0f, 2.0f, 45.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 2.0f, 45.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 0.0f, 45.0f, 0.5f, 0.5f, 0.5f,

            -10.0f, 0.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 0.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            10.0f, 2.0f, -10.0f, 0.5f, 0.5f, 0.5f,
            -10.0f, 2.0f, -10.0f, 0.5f, 0.5f, 0.5f,

            40.0f, 0.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            40.0f, 2.0f, 30.0f, 0.5f, 0.5f, 0.5f,
            40.0f, 2.0f, 45.0f, 0.5f, 0.5f, 0.5f,
            40.0f, 0.0f, 45.0f, 0.5f, 0.5f, 0.5f
        },
        // Parcours 3
        {
            -3.0f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            -3.0f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            -3.0f, 2.0f, 95.0f, 0.5f, 0.5f, 0.5f,
            -3.0f, 0.0f, 95.0f, 0.5f, 0.5f, 0.5f,

            3.0f, 0.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            3.0f, 2.0f, -5.0f, 0.5f, 0.5f, 0.5f,
            3.0f, 2.0f, 95.0f, 0.5f, 0.5f, 0.5f,
            3.0f, 0.0f, 95.0f, 0.5f, 0.5f, 0.5f
        }
    };

    // Configuration initiale du parcours
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * wallVertices[currentCourse].size(), wallVertices[currentCourse].data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void setupCircle()
{
    glGenVertexArrays(1, &circleVAO);
    glBindVertexArray(circleVAO);

    glGenBuffers(1, &circleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);

    const int circleSegments = 36;
    const float circleRadius = 1.2f;

    std::vector<GLfloat> circleVertices;

    // Centre du cercle
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);

    for (int i = 0; i <= circleSegments; ++i)
    {
        float angle = i * 2 * glm::pi<float>() / circleSegments;
        float x = circleRadius * cosf(angle);
        float z = circleRadius * sinf(angle);

        circleVertices.push_back(x);
        circleVertices.push_back(0.0f);
        circleVertices.push_back(z);
    }

    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(GLfloat), circleVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    glBindVertexArray(0);
}

void setupPowerGauge()
{
    glGenVertexArrays(1, &powerGaugeVAO);
    glBindVertexArray(powerGaugeVAO);

    glGenBuffers(1, &powerGaugeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, powerGaugeVBO);

    // Dimensions ajustées pour l'indicateur de puissance (repositionné loin du bord droit)
    GLfloat powerGaugeVertices[] =
    {
        // Positions                // Couleurs
         600.0f, 550.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // En bas à gauche
         600.0f, 580.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // En haut à gauche
         750.0f, 580.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // En haut à droite
         750.0f, 550.0f, 0.0f,      1.0f, 1.0f, 1.0f   // En bas à droite
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(powerGaugeVertices), powerGaugeVertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void setupCylinder()
{
    const int cylinderSegments = 36;
    const float cylinderRadius = 0.1f;
    const float cylinderHeight = 7.0f;

    std::vector<GLfloat> cylinderVertices;
    std::vector<GLuint> cylinderIndices;

    for (int i = 0; i <= cylinderSegments; ++i)
    {
        float angle = i * 2 * glm::pi<float>() / cylinderSegments;
        float x = cylinderRadius * cosf(angle);
        float z = cylinderRadius * sinf(angle);

        // Base inférieure
        cylinderVertices.push_back(x);
        cylinderVertices.push_back(0.0f);
        cylinderVertices.push_back(z);

        // Base supérieure
        cylinderVertices.push_back(x);
        cylinderVertices.push_back(cylinderHeight);
        cylinderVertices.push_back(z);
    }

    for (int i = 0; i < cylinderSegments * 2; i += 2)
    {
        cylinderIndices.push_back(i);
        cylinderIndices.push_back(i + 1);
        cylinderIndices.push_back((i + 2) % (cylinderSegments * 2));

        cylinderIndices.push_back(i + 1);
        cylinderIndices.push_back((i + 3) % (cylinderSegments * 2));
        cylinderIndices.push_back((i + 2) % (cylinderSegments * 2));
    }

    glGenVertexArrays(1, &cylinderVAO);
    glBindVertexArray(cylinderVAO);

    glGenBuffers(1, &cylinderVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, cylinderVertices.size() * sizeof(GLfloat), cylinderVertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &cylinderEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinderEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinderIndices.size() * sizeof(GLuint), cylinderIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    glBindVertexArray(0);
}

void setupFlag()
{
    glGenVertexArrays(1, &flagVAO);
    glBindVertexArray(flagVAO);

    glGenBuffers(1, &flagVBO);
    glBindBuffer(GL_ARRAY_BUFFER, flagVBO);

    // Définir les sommets du drapeau (triangle rouge plus grand et aligné à gauche)
    GLfloat flagVertices[] = {
        // Positions                // Couleurs
        0.0f, 7.0f, 0.0f,          1.0f, 0.0f, 0.0f,  // En bas à gauche
        0.0f, 6.0f, 0.0f,          1.0f, 0.0f, 0.0f,  // En bas à droite
        1.5f, 6.5f, 0.0f,          1.0f, 0.0f, 0.0f   // En haut
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(flagVertices), flagVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E)
    {
        double currentTime = glfwGetTime();
        if (action == GLFW_PRESS)
        {
            keyPressDuration = 0.0;
        }
        else if (action == GLFW_RELEASE)
        {
            if (currentTime - lastShotTime >= shotCooldown)
            {
                float impulseStrength = glm::clamp(static_cast<float>(keyPressDuration / maxKeyPressDuration) * maxImpulseStrength, 0.0f, maxImpulseStrength);

                glm::vec3 cameraDirection(
                    -cos(angleX) * sin(angleY),
                    0.0f, // 1 pour projeter la balle en l'air, 0 vers le sol
                    -cos(angleX) * cos(angleY)
                );

                cameraDirection = glm::normalize(cameraDirection);

                sphereVelocity += cameraDirection * impulseStrength;
                keyPressDuration = 0.0; // Réinitialiser keyPressDuration lorsque la touche est relâchée
                lastShotTime = currentTime; // Mettre à jour le temps du dernier tir
                numShots++; // Incrémenter le nombre de tirs
            }
        }
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        spherePosition = initialSpherePosition;
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        showEndText = false;
        numShots = 0; // Réinitialiser le nombre de tirs
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        cursorLocked = !cursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR, cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    else if (key == GLFW_KEY_KP_1 && action == GLFW_PRESS) // Téléportation au parcours 1
    {
        currentCourse = 0;
        initialSpherePosition = glm::vec3(0.0f, radius, 0.0f);
        spherePosition = initialSpherePosition;
        trailPositions.clear(); // Effacer la traînée
        setupGround(); // Recharger le sol pour le nouveau parcours
        setupWalls(); // Recharger les murs pour le nouveau parcours
        showEndText = false;
    }
    else if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) // Téléportation au parcours 2
    {
        currentCourse = 1;
        initialSpherePosition = glm::vec3(-5.0f, radius, -5.0f);
        spherePosition = initialSpherePosition;
        trailPositions.clear(); // Effacer la traînée
        setupGround(); // Recharger le sol pour le nouveau parcours
        setupWalls(); // Recharger les murs pour le nouveau parcours
        showEndText = false;
    }
    else if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS) // Téléportation au parcours 3
    {
        currentCourse = 2;
        initialSpherePosition = glm::vec3(0.0f, radius, 0.0f);
        spherePosition = initialSpherePosition;
        trailPositions.clear(); // Effacer la traînée
        setupGround(); // Recharger le sol pour le nouveau parcours
        setupWalls(); // Recharger les murs pour le nouveau parcours
        showEndText = false;
    }
}

GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    }
    else
    {
        std::cerr << "Impossible d'ouvrir " << vertex_file_path << ". Êtes-vous dans le bon répertoire ?" << std::endl;
        getchar();
        return 0;
    }

    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open())
    {
        std::stringstream sstr;
        sstr << FragmentShaderStream.rdbuf();
        FragmentShaderCode = sstr.str();
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    std::cout << "Compilation du shader: " << vertex_file_path << std::endl;
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        std::cerr << &VertexShaderErrorMessage[0] << std::endl;
    }

    std::cout << "Compilation du shader: " << fragment_file_path << std::endl;
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        std::cerr << &FragmentShaderErrorMessage[0] << std::endl;
    }

    std::cout << "Lien du programme" << std::endl;
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0)
    {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr << &ProgramErrorMessage[0] << std::endl;
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

bool init()
{
    if (!glfwInit())
    {
        std::cerr << "Échec de l'initialisation de GLFW" << std::endl;
        return false;
    }

    window = glfwCreateWindow(1080, 720, "Golf 3D", NULL, NULL);
    if (!window)
    {
        std::cerr << "Échec de la création de la fenêtre GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Échec de l'initialisation de GLEW" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    setupSphere();
    setupGround();
    setupWalls();
    setupCylinder();
    setupCircle();
    setupPowerGauge();
    setupFlag();

    ballShaderProgram = loadShaders("ball_vertex_shader.glsl", "ball_fragment_shader.glsl");
    shaderProgram = loadShaders("vertex_shader.glsl", "fragment_shader.glsl");
    circleShaderProgram = loadShaders("circle_vertex_shader.glsl", "circle_fragment_shader.glsl");
    poleShaderProgram = loadShaders("pole_vertex_shader.glsl", "pole_fragment_shader.glsl");
    flagShaderProgram = loadShaders("flag_vertex_shader.glsl", "flag_fragment_shader.glsl");
    gaugeShaderProgram = loadShaders("gauge_vertex_shader.glsl", "gauge_fragment_shader.glsl");
    trailShaderProgram = loadShaders("trail_vertex_shader.glsl", "trail_fragment_shader.glsl");

    return true;
}

void updatePowerGauge(float powerRatio)
{
    glBindVertexArray(powerGaugeVAO);

    // Calculer la couleur en fonction du rapport de puissance
    glm::vec3 color;
    double currentTime = glfwGetTime();
    if (currentTime - lastShotTime < shotCooldown)
    {
        color = glm::vec3(0.5f, 0.5f, 0.5f); // Couleur grise pendant le temps de récupération
    }
    else
    {
        if (powerRatio < 0.25f)
            color = glm::mix(glm::vec3(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), powerRatio * 4.0f);  // Blanc à Vert
        else if (powerRatio < 0.5f)
            color = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), (powerRatio - 0.25f) * 4.0f);  // Vert à Jaune
        else if (powerRatio < 0.75f)
            color = glm::mix(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.0f), (powerRatio - 0.5f) * 4.0f);  // Jaune à Orange
        else
            color = glm::mix(glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), (powerRatio - 0.75f) * 4.0f);  // Orange à Rouge
    }

    GLfloat powerGaugeVertices[] =
    {
        // Positions              // Couleurs
         600.0f, 550.0f, 0.0f,     color.r, color.g, color.b,  // En bas à gauche
         600.0f, 580.0f, 0.0f,     color.r, color.g, color.b,  // En haut à gauche
         750.0f, 580.0f, 0.0f,     color.r, color.g, color.b,  // En haut à droite
         750.0f, 550.0f, 0.0f,     color.r, color.g, color.b   // En bas à droite
    };

    glBindBuffer(GL_ARRAY_BUFFER, powerGaugeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(powerGaugeVertices), powerGaugeVertices);

    glBindVertexArray(0);
}

void updateBallRotation(float deltaTime)
{
    if (glm::length(sphereVelocity) > 0.0f)
    {
        glm::vec3 rotationAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), sphereVelocity));
        float rotationAngle = glm::length(sphereVelocity) * deltaTime / radius * rotationSpeedFactor; // Augmenter la vitesse de rotation

        ballRotation = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis) * ballRotation;
    }
}

void drawPowerGauge()
{
    glUseProgram(gaugeShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

    GLuint modelLoc = glGetUniformLocation(gaugeShaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(gaugeShaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(gaugeShaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(powerGaugeVAO);
    glDrawArrays(GL_QUADS, 0, 4);
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawSphere()
{
    glUseProgram(ballShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, spherePosition) * ballRotation; // Appliquer la rotation

    GLuint modelLoc = glGetUniformLocation(ballShaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(ballShaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(ballShaderProgram, "projection");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) - .0f,
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(sphereVAO);
    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            glArrayElement(k1);
            glArrayElement(k2);
            glArrayElement(k1 + 1);

            glArrayElement(k1 + 1);
            glArrayElement(k2);
            glArrayElement(k2 + 1);
        }
        glEnd();
    }
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawGround()
{
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f,
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(groundVAO);
    glDrawArrays(GL_QUADS, 0, (currentCourse == 2 ? 4 : 8)); // Mettre à jour le nombre de sommets
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawWalls()
{
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f,
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(wallVAO);
    glDrawArrays(GL_QUADS, 0, (currentCourse == 2 ? 8 : 28)); // Mettre à jour le nombre de sommets
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawCircle()
{
    glUseProgram(circleShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 holePosition;
    if (currentCourse == 0)
        holePosition = glm::vec3(60.0f, 0.02f, 60.0f);
    else if (currentCourse == 1)
        holePosition = glm::vec3(35.0f, 0.02f, 37.5f);
    else
        holePosition = glm::vec3(0.0f, 0.02f, 90.0f);
    model = glm::translate(model, holePosition);

    GLuint modelLoc = glGetUniformLocation(circleShaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(circleShaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(circleShaderProgram, "projection");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f,
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 38);
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawCylinder()
{
    // Dessiner le cylindre
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 cylinderPosition;
    if (currentCourse == 0)
        cylinderPosition = glm::vec3(60.0f, 0.0f, 60.0f);
    else if (currentCourse == 1)
        cylinderPosition = glm::vec3(35.0f, 0.0f, 37.5f);
    else
        cylinderPosition = glm::vec3(0.0f, 0.0f, 90.0f);
    model = glm::translate(model, cylinderPosition);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f,
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(cylinderVAO);
    glDrawElements(GL_TRIANGLES, 36 * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    // Dessiner le drapeau
    glUseProgram(flagShaderProgram); // Utiliser le nouveau programme de shader pour le drapeau

    model = glm::mat4(1.0f);
    model = glm::translate(model, cylinderPosition);

    modelLoc = glGetUniformLocation(flagShaderProgram, "model");
    viewLoc = glGetUniformLocation(flagShaderProgram, "view");
    projLoc = glGetUniformLocation(flagShaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(flagVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawTrail()
{
    glUseProgram(trailShaderProgram);

    for (size_t i = 0; i < trailPositions.size(); ++i)
    {
        glm::vec3 position = trailPositions[i];
        float scale = 0.5f * (float)i / maxTrailLength; // Sphères plus grandes pour les nouvelles positions, plus petites pour les anciennes
        glm::vec4 color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f - (float)i / maxTrailLength); // Couleur cyan avec effet de dégradé

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(scale));

        GLuint modelLoc = glGetUniformLocation(trailShaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(trailShaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(trailShaderProgram, "projection");
        GLuint colorLoc = glGetUniformLocation(trailShaderProgram, "color");

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

        glm::vec3 cameraPosition = spherePosition + glm::vec3(
            zoom * cos(angleX) * sin(angleY),
            zoom * sin(angleX) + 2.0f,
            zoom * cos(angleX) * cos(angleY)
        );
        glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform4fv(colorLoc, 1, glm::value_ptr(color));

        glBindVertexArray(sphereVAO);
        for (int j = 0; j < stackCount; ++j)
        {
            int k1 = j * (sectorCount + 1);
            int k2 = k1 + sectorCount + 1;

            glBegin(GL_TRIANGLE_STRIP);
            for (int l = 0; l < sectorCount; ++l, ++k1, ++k2)
            {
                glArrayElement(k1);
                glArrayElement(k2);
                glArrayElement(k1 + 1);

                glArrayElement(k1 + 1);
                glArrayElement(k2);
                glArrayElement(k2 + 1);
            }
            glEnd();
        }
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void checkSphereBounds() {
    // Définir les limites pour chaque parcours
    std::vector<std::vector<float>> boundaries = {
        // Parcours 1
        { -5.5f, 5.5f, 65.5f, -5.0f, 50.0f, 65.0f },
        // Parcours 2
        { -10.0f, 10.0f, 40.0f, -10.0f, 30.0f, 45.0f },
        // Parcours 3
        { -3.0f, 3.0f, -5.0f, 95.0f }  // { minX, maxX, minZ, maxZ }
    };

    auto& bounds = boundaries[currentCourse];

    if (currentCourse != 2) {
        // Vérifier les collisions avec les murs latéraux de la section principale
        if (spherePosition.x <= bounds[0] + radius && (spherePosition.z >= bounds[3] + radius && spherePosition.z <= bounds[4] - radius)) {
            spherePosition.x = bounds[0] + radius; // Repositionner la sphère
            sphereVelocity.x *= -dampingFactor;
            if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                sphereVelocity.x = 0.0f;
            }
        }
        else if (spherePosition.x >= bounds[1] - radius && (spherePosition.z >= bounds[3] + radius && spherePosition.z <= bounds[4] - radius)) {
            spherePosition.x = bounds[1] - radius; // Repositionner la sphère
            sphereVelocity.x *= -dampingFactor;
            if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                sphereVelocity.x = 0.0f;
            }
        }

        // Vérifier les collisions avec les murs de la section ajoutée pour former la forme en L (Parcours 1 et Parcours 2 seulement)
        if (currentCourse != 2) {
            if (spherePosition.x >= bounds[2] - radius && (spherePosition.z >= bounds[4] + radius && spherePosition.z <= bounds[5] - radius)) {
                spherePosition.x = bounds[2] - radius; // Repositionner la sphère
                sphereVelocity.x *= -dampingFactor;
                if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                    sphereVelocity.x = 0.0f;
                }
            }

            if (spherePosition.z >= bounds[5] - radius && (spherePosition.x >= bounds[0] + radius && spherePosition.x <= bounds[2] - radius)) {
                spherePosition.z = bounds[5] - radius; // Repositionner la sphère
                sphereVelocity.z *= -dampingFactor;
                if (std::abs(sphereVelocity.z) < minBounceSpeed) {
                    sphereVelocity.z = 0.0f;
                }
            }

            // Vérifier les collisions avec les murs latéraux de la section ajoutée (partie horizontale)
            if (spherePosition.z >= bounds[4] && (spherePosition.x <= bounds[0] + radius)) {
                spherePosition.x = bounds[0] + radius; // Repositionner la sphère
                sphereVelocity.x *= -dampingFactor;
                if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                    sphereVelocity.x = 0.0f;
                }
            }
            else if (spherePosition.z >= bounds[4] && (spherePosition.x >= bounds[2] - radius)) {
                spherePosition.x = bounds[2] - radius; // Repositionner la sphère
                sphereVelocity.x *= -dampingFactor;
                if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                    sphereVelocity.x = 0.0f;
                }
            }
        }

        // Vérifier les collisions avec les murs au début de la section principale
        if (spherePosition.z <= bounds[3] + radius) {
            spherePosition.z = bounds[3] + radius; // Repositionner la sphère
            sphereVelocity.z *= -dampingFactor;
            if (std::abs(sphereVelocity.z) < minBounceSpeed) {
                sphereVelocity.z = 0.0f;
            }
        }
    }
    else {
        // Vérifications spécifiques pour les limites du parcours 3
        if (spherePosition.x <= bounds[0] + radius) {
            spherePosition.x = bounds[0] + radius; // Repositionner la sphère
            sphereVelocity.x *= -dampingFactor;
            if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                sphereVelocity.x = 0.0f;
            }
        }
        else if (spherePosition.x >= bounds[1] - radius) {
            spherePosition.x = bounds[1] - radius; // Repositionner la sphère
            sphereVelocity.x *= -dampingFactor;
            if (std::abs(sphereVelocity.x) < minBounceSpeed) {
                sphereVelocity.x = 0.0f;
            }
        }

        if (spherePosition.z <= bounds[2] + radius) {
            spherePosition.z = bounds[2] + radius; // Repositionner la sphère
            sphereVelocity.z *= -dampingFactor;
            if (std::abs(sphereVelocity.z) < minBounceSpeed) {
                sphereVelocity.z = 0.0f;
            }
        }
        else if (spherePosition.z >= bounds[3] - radius) {
            spherePosition.z = bounds[3] - radius; // Repositionner la sphère
            sphereVelocity.z *= -dampingFactor;
            if (std::abs(sphereVelocity.z) < minBounceSpeed) {
                sphereVelocity.z = 0.0f;
            }
        }
    }

    // Vérifier les collisions avec le sol
    if (spherePosition.y <= radius) {
        spherePosition.y = radius;
        sphereVelocity.y *= -dampingFactor;
        if (std::abs(sphereVelocity.y) < minBounceSpeed) {
            sphereVelocity.y = 0.0f;
        }
    }
}



bool checkHoleCollision()
{
    float holeRadius = 1.5f;
    glm::vec3 holePosition;
    if (currentCourse == 0)
        holePosition = glm::vec3(60.0f, 0.0f, 60.0f);
    else if (currentCourse == 1)
        holePosition = glm::vec3(35.0f, 0.0f, 37.5f);
    else
        holePosition = glm::vec3(0.0f, 0.0f, 90.0f);

    float distance = glm::distance(spherePosition, holePosition);
    if (distance < holeRadius)
    {
        std::cout << "Parcours terminé !" << std::endl;
        lastShotTime = glfwGetTime() - shotCooldown; // Réinitialiser le temps de récupération du tir
        levelTransition = true;
        endTime = glfwGetTime();
        return true;
    }
    return false;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Mettre à jour les positions de la traînée
    if (trailPositions.size() >= maxTrailLength)
    {
        trailPositions.erase(trailPositions.begin()); // Supprimer la position la plus ancienne si nous dépassons la longueur maximale de la traînée
    }
    trailPositions.push_back(spherePosition); // Ajouter la position actuelle à la traînée

    drawGround();
    drawWalls();
    drawTrail(); // Dessiner la traînée avant de dessiner la sphère
    drawCircle();
    drawCylinder();

    for (int i = 0; i < subSteps; ++i)
    {
        spherePosition.y += sphereVelocity.y / subSteps;
        spherePosition.x += sphereVelocity.x / subSteps;
        spherePosition.z += sphereVelocity.z / subSteps;

        sphereVelocity *= pow(friction, 1.0f / subSteps); // Appliquer la friction par sous-étape

        sphereVelocity.y -= gravity / subSteps;

        // Vérifier si la sphère est au-dessus du sol
        if (spherePosition.y <= radius)
        {
            spherePosition.y = radius;
            sphereVelocity.y *= -dampingFactor;
            if (std::abs(sphereVelocity.y) < minBounceSpeed)
            {
                sphereVelocity.y = 0.0f;
            }
        }

        // Vérifier les collisions avec les murs
        checkSphereBounds();
    }

    // Vérifier si la sphère est entrée dans le trou
    if (checkHoleCollision())
    {
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    drawSphere();
    drawPowerGauge();

    if (levelTransition && glfwGetTime() - endTime > levelTransitionDelay)
    {
        // Attendre le délai de transition
        levelTransition = false;
        currentCourse = (currentCourse + 1) % 3; // Passer au parcours suivant (y compris le troisième parcours)
        initialSpherePosition = currentCourse == 0 ? glm::vec3(0.0f, radius, 0.0f) : (currentCourse == 1 ? glm::vec3(-5.0f, radius, -5.0f) : glm::vec3(0.0f, radius, 0.0f));
        spherePosition = initialSpherePosition;
        trailPositions.clear(); // Effacer la traînée
        setupGround(); // Recharger le sol pour le nouveau parcours
        setupWalls(); // Recharger les murs pour le nouveau parcours
        showEndText = false;
    }

    if (showEndText && glfwGetTime() - endTime > 3.0)
    {
        // Attendre 3 secondes
        spherePosition = initialSpherePosition;
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        showEndText = false;
    }

    glfwSwapBuffers(window);
}

int main()
{
    if (!init())
        return -1;

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            keyPressDuration += deltaTime;
            keyPressDuration = std::min(keyPressDuration, maxKeyPressDuration);
        }

        updatePowerGauge(static_cast<float>(keyPressDuration / maxKeyPressDuration));
        updateBallRotation(deltaTime);

        draw();
    }

    glfwTerminate();
    return 0;
}
