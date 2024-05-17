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
double lastX = 400.0; // Initialiser au centre de la fenêtre
double lastY = 300.0; // Initialiser au centre de la fenêtre
const float sensitivity = 0.005f; // Sensibilité de la souris réduite

GLuint sphereVAO, sphereVBO, groundVAO, groundVBO, wallVAO, wallVBO;
GLuint cylinderVAO, cylinderVBO, cylinderEBO;
GLuint circleVAO, circleVBO;
GLuint powerGaugeVAO, powerGaugeVBO; // Ajoutez ces lignes
GLuint shaderProgram, circleShaderProgram, gaugeShaderProgram; // Ajoutez `gaugeShaderProgram`

const int sectorCount = 36;
const int stackCount = 18;
const float radius = 0.6f;
const float dampingFactor = 0.8f;
const float gravity = 0.005f;
const float minBounceSpeed = 0.035f;
const float friction = 0.995f; // Facteur de friction

double keyPressDuration = 0.0;
const double maxKeyPressDuration = 3.0;
const float maxImpulseStrength = 2.0f;
double lastTime = glfwGetTime();
double endTime = 0.0;
bool showEndText = false;

glm::vec3 initialSpherePosition(0.0f, radius, 0.0f); // Position initiale sur le sol
glm::vec3 spherePosition = initialSpherePosition;
glm::vec3 sphereVelocity(0.0f, 0.0f, 0.0f);

bool cursorLocked = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;
    angleY += deltaX * sensitivity; // Utiliser la sensibilité
    angleX += deltaY * sensitivity; // Utiliser la sensibilité
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E)
    {
        if (action == GLFW_PRESS)
        {
            keyPressDuration = 0.0;
        }
        else if (action == GLFW_RELEASE)
        {
            float impulseStrength = glm::clamp(static_cast<float>(keyPressDuration / maxKeyPressDuration) * maxImpulseStrength, 0.0f, maxImpulseStrength);

            glm::vec3 cameraDirection(
                -cos(angleX) * sin(angleY),
                0.0f, // Assurez-vous que la composante y est 0 pour éviter de projeter la balle en l'air
                -cos(angleX) * cos(angleY)
            );

            cameraDirection = glm::normalize(cameraDirection);

            sphereVelocity += cameraDirection * impulseStrength;
            keyPressDuration = 0.0; // Reset the keyPressDuration when the key is released
        }
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        spherePosition = initialSpherePosition;
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        showEndText = false;
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        cursorLocked = !cursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR, cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

void setupSphere()
{
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);

    std::vector<GLfloat> sphereVertices;
    std::vector<GLfloat> sphereColors;
    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stackCount;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        float y = 1.0f;
        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2 * glm::pi<float>() / sectorCount;
            float x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle) + 0.0f;
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);

            sphereColors.push_back(1.0f);
            sphereColors.push_back(1.0f);
            sphereColors.push_back(1.0f);
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

    glBindVertexArray(0);
}

void setupGround()
{
    glGenVertexArrays(1, &groundVAO);
    glBindVertexArray(groundVAO);

    glGenBuffers(1, &groundVBO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);

    // Terrain de golf élargi
    GLfloat groundVertices[] =
    {
        -5.5f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,
        -5.5f, 0.0f, 50.0f,   0.0f, 1.0f, 0.0f,
         5.5f, 0.0f, 50.0f,   0.0f, 1.0f, 0.0f,
         5.5f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

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

    // Murets autour du terrain élargi
    GLfloat wallVertices[] =
    {
        // Murets de gauche
        -5.5f, 0.0f, -5.0f,   0.5f, 0.5f, 0.5f,
        -5.5f, 2.0f, -5.0f,   0.5f, 0.5f, 0.5f,
        -5.5f, 2.0f, 50.0f,   0.5f, 0.5f, 0.5f,
        -5.5f, 0.0f, 50.0f,   0.5f, 0.5f, 0.5f,

        // Murets de droite
         5.5f, 0.0f, -5.0f,   0.5f, 0.5f, 0.5f,
         5.5f, 2.0f, -5.0f,   0.5f, 0.5f, 0.5f,
         5.5f, 2.0f, 50.0f,   0.5f, 0.5f, 0.5f,
         5.5f, 0.0f, 50.0f,   0.5f, 0.5f, 0.5f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);

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
    const float circleRadius = 1.2f; // Agrandir le cercle

    std::vector<GLfloat> circleVertices;

    // Centre du cercle
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f); // Légèrement au-dessus du sol
    circleVertices.push_back(0.0f);

    for (int i = 0; i <= circleSegments; ++i)
    {
        float angle = i * 2 * glm::pi<float>() / circleSegments;
        float x = circleRadius * cosf(angle);
        float z = circleRadius * sinf(angle);

        circleVertices.push_back(x);
        circleVertices.push_back(0.0f); // Légèrement au-dessus du sol
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

    // Initial vertices for the power gauge (position and color)
    GLfloat powerGaugeVertices[] = 
    {
        // Positions            // Colors
         0.0f, 0.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // Bottom-left
         0.0f, 20.0f, 0.0f,     1.0f, 1.0f, 1.0f,  // Top-left
         100.0f, 20.0f, 0.0f,   1.0f, 1.0f, 1.0f,  // Top-right
         100.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f   // Bottom-right
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
    const float cylinderRadius = 0.1f; // Rayon fin du poteau
    const float cylinderHeight = 7.0f; // Longueur du poteau

    std::vector<GLfloat> cylinderVertices;
    std::vector<GLuint> cylinderIndices;

    // Ajouter les sommets et indices pour le cylindre
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
        std::cerr << "Impossible to open " << vertex_file_path << ". Are you in the right directory ?" << std::endl;
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

    std::cout << "Compiling shader : " << vertex_file_path << std::endl;
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

    std::cout << "Compiling shader : " << fragment_file_path << std::endl;
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

    std::cout << "Linking program" << std::endl;
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
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    window = glfwCreateWindow(800, 600, "Golf 3D", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    setupSphere();
    setupGround();
    setupWalls();
    setupCylinder();
    setupCircle();
    setupPowerGauge(); // Initialisez la jauge de puissance

    shaderProgram = loadShaders("vertex_shader.glsl", "fragment_shader.glsl");
    circleShaderProgram = loadShaders("circle_vertex_shader.glsl", "circle_fragment_shader.glsl");
    gaugeShaderProgram = loadShaders("gauge_vertex_shader.glsl", "gauge_fragment_shader.glsl"); // Chargez le shader de la jauge

    return true;
}

void updatePowerGauge(float powerRatio)
{
    glBindVertexArray(powerGaugeVAO);

    // Calculate color based on power ratio
    glm::vec3 color;
    if (powerRatio < 0.25f)
        color = glm::mix(glm::vec3(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), powerRatio * 4.0f);  // White to Green
    else if (powerRatio < 0.5f)
        color = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), (powerRatio - 0.25f) * 4.0f);  // Green to Yellow
    else if (powerRatio < 0.75f)
        color = glm::mix(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.0f), (powerRatio - 0.5f) * 4.0f);  // Yellow to Orange
    else
        color = glm::mix(glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), (powerRatio - 0.75f) * 4.0f);  // Orange to Red

    GLfloat powerGaugeVertices[] = 
    {
        // Positions                // Colors
         690.0f, 570.0f, 0.0f,      color.r, color.g, color.b,  // Bottom-left
         690.0f, 590.0f, 0.0f,      color.r, color.g, color.b,  // Top-left
         790.0f, 590.0f, 0.0f,      color.r, color.g, color.b,  // Top-right
         790.0f, 570.0f, 0.0f,      color.r, color.g, color.b   // Bottom-right
    };

    glBindBuffer(GL_ARRAY_BUFFER, powerGaugeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(powerGaugeVertices), powerGaugeVertices);

    glBindVertexArray(0);
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
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, spherePosition);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // Décalage vers le bas pour la caméra
    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) - .0f,  // Décalage vers le bas
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
        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
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

    glUseProgram(0);
}


void drawGround()
{
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f, // Déplacez la caméra pour que la balle ne soit pas au centre de l'écran
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(groundVAO);
    glDrawArrays(GL_QUADS, 0, 4); // Mettre à jour pour dessiner la nouvelle section
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

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f, // Déplacez la caméra pour que la balle ne soit pas au centre de l'écran
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(wallVAO);
    glDrawArrays(GL_QUADS, 0, 8); // Dessiner les murets
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawCircle()
{
    glUseProgram(circleShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.02f, 50.0f)); // Positionner le cercle légèrement au-dessus du sol

    GLuint modelLoc = glGetUniformLocation(circleShaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(circleShaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(circleShaderProgram, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f, // Déplacez la caméra pour que la balle ne soit pas au centre de l'écran
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 38); // Dessiner le cercle rempli
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawCylinder()
{
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 50.0f)); // Positionner le cylindre au milieu du cercle

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::vec3 cameraPosition = spherePosition + glm::vec3(
        zoom * cos(angleX) * sin(angleY),
        zoom * sin(angleX) + 2.0f, // Déplacez la caméra pour que la balle ne soit pas au centre de l'écran
        zoom * cos(angleX) * cos(angleY)
    );
    glm::mat4 view = glm::lookAt(cameraPosition, spherePosition, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(cylinderVAO);
    glDrawElements(GL_TRIANGLES, 36 * 6, GL_UNSIGNED_INT, 0); // Dessiner le cylindre
    glBindVertexArray(0);

    glUseProgram(0);
}

bool checkHoleCollision()
{
    const float holeRadius = 1.5f; // Agrandir le trou
    glm::vec3 holePosition(0.0f, 0.0f, 50.0f); // Positionner le trou à la fin du terrain

    float distance = glm::distance(spherePosition, holePosition);
    if (distance < holeRadius)
    {
        std::cout << "Parcours termine !" << std::endl; // Ligne de debug
        return true;
    }
    return false;
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawGround();
    drawWalls();
    drawCircle(); // Ajouter le dessin du cercle
    drawCylinder(); // Ajouter le dessin du cylindre

    spherePosition.y += sphereVelocity.y;
    spherePosition.x += sphereVelocity.x;
    spherePosition.z += sphereVelocity.z;

    sphereVelocity *= friction; // Appliquer la friction

    sphereVelocity.y -= gravity;

    // Check if the sphere is above the ground
    if (spherePosition.y <= radius)
    {
        // Vérifier les limites de la section du terrain élargi
        if (spherePosition.x >= -5.5f && spherePosition.x <= 5.5f && spherePosition.z >= -5.0f && spherePosition.z <= 50.0f)
        {
            spherePosition.y = radius;
            sphereVelocity.y *= -dampingFactor;
            if (std::abs(sphereVelocity.y) < minBounceSpeed)
            {
                sphereVelocity.y = 0.0f;
            }
        }
    }

    // Check if the sphere hits the walls
    if (spherePosition.x <= -5.5f || spherePosition.x >= 5.5f)
    {
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }

    if (spherePosition.z <= -5.0f || spherePosition.z >= 50.0f)
    {
        sphereVelocity.z *= -dampingFactor;
        if (std::abs(sphereVelocity.z) < minBounceSpeed)
        {
            sphereVelocity.z = 0.0f;
        }
    }

    // Check if the sphere entered the hole
    if (checkHoleCollision())
    {
        if (!showEndText)
        {
            endTime = glfwGetTime();
            showEndText = true;
        }
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    drawSphere();
    drawPowerGauge(); // Dessiner la jauge de puissance

    if (showEndText && glfwGetTime() - endTime > 3.0) // Wait for 3 seconds
    {
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

        draw();
    }

    glfwTerminate();
    return 0;
}
