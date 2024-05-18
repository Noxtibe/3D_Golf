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
const float sensitivity = 0.005f; // Mouse sensitivity

GLuint sphereVAO, sphereVBO, groundVAO, groundVBO, wallVAO, wallVBO;
GLuint cylinderVAO, cylinderVBO, cylinderEBO;
GLuint circleVAO, circleVBO;
GLuint powerGaugeVAO, powerGaugeVBO;
GLuint flagVAO, flagVBO;
GLuint shaderProgram, circleShaderProgram, gaugeShaderProgram; // Shaders

const int sectorCount = 36;
const int stackCount = 18;
const float radius = 0.6f;
const float dampingFactor = 0.8f;
const float gravity = 0.005f;
const float minBounceSpeed = 0.001f;
const float friction = 0.995f; // Friction
const int subSteps = 10; // Number of sub-steps for simulation

double keyPressDuration = 0.0;
const double maxKeyPressDuration = 3.0;
const float maxImpulseStrength = 2.0f;
double lastShotTime = -2.0; // Initialize to -2 to allow the first shot immediately
const double shotCooldown = 2.0; // Cooldown time in seconds
double lastTime = glfwGetTime();
double endTime = 0.0;
bool showEndText = false;

const int maxTrailLength = 50; // Maximum length of the trail
std::vector<glm::vec3> trailPositions; // Vector to store trail positions

const double levelTransitionDelay = 0.0; // Delay before transitioning to the next level
bool levelTransition = false; // Flag to indicate level transition


glm::vec3 initialSpherePosition(0.0f, radius, 0.0f); // Initial position on the ground
glm::vec3 spherePosition = initialSpherePosition;
glm::vec3 sphereVelocity(0.0f, 0.0f, 0.0f);

bool cursorLocked = true;
int currentCourse = 0; // Variable to keep track of the current course

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
                    0.0f, // 1 to project the ball in the air, 0 to ground
                    -cos(angleX) * cos(angleY)
                );

                cameraDirection = glm::normalize(cameraDirection);

                sphereVelocity += cameraDirection * impulseStrength;
                keyPressDuration = 0.0; // Reset the keyPressDuration when the key is released
                lastShotTime = currentTime; // Update the last shot time
            }
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

    // Define ground vertices for multiple courses
    std::vector<std::vector<GLfloat>> groundVertices = {
        // Course 1
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
        // Course 2
        {
            -10.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            10.0f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            40.0f, 0.0f, 30.0f, 0.0f, 1.0f, 0.0f,
            40.0f, 0.0f, 45.0f, 0.0f, 1.0f, 0.0f,
            -10.0f, 0.0f, 45.0f, 0.0f, 1.0f, 0.0f
        }
    };

    // Initial course setup
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

    // Define wall vertices for multiple courses
    std::vector<std::vector<GLfloat>> wallVertices = {
        // Course 1
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
        // Course 2
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
        }
    };

    // Initial course setup
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

    // Circle center
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

    // Adjusted dimensions for the power gauge (repositioned away from the right edge)
    GLfloat powerGaugeVertices[] =
    {
        // Positions                // Colors
         600.0f, 550.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // Bottom-left
         600.0f, 580.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // Top-left
         750.0f, 580.0f, 0.0f,      1.0f, 1.0f, 1.0f,  // Top-right
         750.0f, 550.0f, 0.0f,      1.0f, 1.0f, 1.0f   // Bottom-right
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

        // Inferior base
        cylinderVertices.push_back(x);
        cylinderVertices.push_back(0.0f);
        cylinderVertices.push_back(z);

        // Superior base
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

    // Define flag vertices (larger and aligned red triangle on the left side)
    GLfloat flagVertices[] = {
        // Positions                // Colors
        0.0f, 7.0f, 0.0f,          1.0f, 0.0f, 0.0f,  // Bottom-left
        0.0f, 6.0f, 0.0f,          1.0f, 0.0f, 0.0f,  // Bottom-right
        1.5f, 6.5f, 0.0f,         1.0f, 0.0f, 0.0f   // Top
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(flagVertices), flagVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

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

    std::cout << "Compiling shader: " << vertex_file_path << std::endl;
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

    std::cout << "Compiling shader: " << fragment_file_path << std::endl;
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

bool init() {
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    window = glfwCreateWindow(1080, 720, "Golf 3D", NULL, NULL);
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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
    setupPowerGauge();
    setupFlag(); // Add this line

    shaderProgram = loadShaders("vertex_shader.glsl", "fragment_shader.glsl");
    circleShaderProgram = loadShaders("circle_vertex_shader.glsl", "circle_fragment_shader.glsl");
    gaugeShaderProgram = loadShaders("gauge_vertex_shader.glsl", "gauge_fragment_shader.glsl");

    return true;
}


void updatePowerGauge(float powerRatio)
{
    glBindVertexArray(powerGaugeVAO);

    // Calculate color based on power ratio
    glm::vec3 color;
    double currentTime = glfwGetTime();
    if (currentTime - lastShotTime < shotCooldown)
    {
        color = glm::vec3(0.5f, 0.5f, 0.5f); // Grey color during cooldown
    }
    else
    {
        if (powerRatio < 0.25f)
            color = glm::mix(glm::vec3(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), powerRatio * 4.0f);  // White to Green
        else if (powerRatio < 0.5f)
            color = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), (powerRatio - 0.25f) * 4.0f);  // Green to Yellow
        else if (powerRatio < 0.75f)
            color = glm::mix(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.5f, 0.0f), (powerRatio - 0.5f) * 4.0f);  // Yellow to Orange
        else
            color = glm::mix(glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), (powerRatio - 0.75f) * 4.0f);  // Orange to Red
    }

    GLfloat powerGaugeVertices[] =
    {
        // Positions              // Colors
         600.0f, 550.0f, 0.0f,     color.r, color.g, color.b,  // Bottom-left
         600.0f, 580.0f, 0.0f,     color.r, color.g, color.b,  // Top-left
         750.0f, 580.0f, 0.0f,     color.r, color.g, color.b,  // Top-right
         750.0f, 550.0f, 0.0f,     color.r, color.g, color.b   // Bottom-right
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
    glDrawArrays(GL_QUADS, 0, 8); // Update number of vertices
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
    glDrawArrays(GL_QUADS, 0, 28); // Update number of vertices
    glBindVertexArray(0);

    glUseProgram(0);
}

void drawCircle()
{
    glUseProgram(circleShaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 holePosition = currentCourse == 0 ? glm::vec3(60.0f, 0.02f, 60.0f) : glm::vec3(35.0f, 0.02f, 37.5f);
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
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 cylinderPosition = currentCourse == 0 ? glm::vec3(60.0f, 0.0f, 60.0f) : glm::vec3(35.0f, 0.0f, 37.5f);
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

    // Draw the cylinder
    glBindVertexArray(cylinderVAO);
    glDrawElements(GL_TRIANGLES, 36 * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Draw the flag
    glBindVertexArray(flagVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glUseProgram(0);
}


bool checkHoleCollision()
{
    float holeRadius = 1.5f;
    glm::vec3 holePosition = currentCourse == 0 ? glm::vec3(60.0f, 0.0f, 60.0f) : glm::vec3(35.0f, 0.0f, 37.5f);

    float distance = glm::distance(spherePosition, holePosition);
    if (distance < holeRadius)
    {
        std::cout << "Course completed!" << std::endl;
        lastShotTime = glfwGetTime() - shotCooldown; // Reset shot cooldown
        levelTransition = true;
        endTime = glfwGetTime();
        return true;
    }
    return false;
}


void checkSphereBounds()
{
    // Define boundaries for each course
    std::vector<std::vector<float>> boundaries = {
        // Course 1
        { -5.5f, 5.5f, 65.5f, -5.0f, 50.0f, 65.0f },
        // Course 2
        { -10.0f, 10.0f, 40.0f, -10.0f, 30.0f, 45.0f }
    };

    auto& bounds = boundaries[currentCourse];

    // Check for collisions with the side walls of the main section
    if (spherePosition.x <= bounds[0] + radius && (spherePosition.z >= bounds[3] + radius && spherePosition.z <= bounds[4] - radius))
    {
        spherePosition.x = bounds[0] + radius; // Reposition the sphere
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }
    else if (spherePosition.x >= bounds[1] - radius && (spherePosition.z >= bounds[3] + radius && spherePosition.z <= bounds[4] - radius))
    {
        spherePosition.x = bounds[1] - radius; // Reposition the sphere
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }

    // Check for collisions with the walls of the section added to form the extended L-shape
    if (spherePosition.x >= bounds[2] - radius && (spherePosition.z >= bounds[4] + radius && spherePosition.z <= bounds[5] - radius))
    {
        spherePosition.x = bounds[2] - radius; // Reposition the sphere
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }

    if (spherePosition.z >= bounds[5] - radius && (spherePosition.x >= bounds[0] + radius && spherePosition.x <= bounds[2] - radius))
    {
        spherePosition.z = bounds[5] - radius; // Reposition the sphere
        sphereVelocity.z *= -dampingFactor;
        if (std::abs(sphereVelocity.z) < minBounceSpeed)
        {
            sphereVelocity.z = 0.0f;
        }
    }

    // Check for collisions with the side walls of the added section (horizontal part)
    if (spherePosition.z >= bounds[4] && (spherePosition.x <= bounds[0] + radius))
    {
        spherePosition.x = bounds[0] + radius; // Reposition the sphere
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }
    else if (spherePosition.z >= bounds[4] && (spherePosition.x >= bounds[2] - radius))
    {
        spherePosition.x = bounds[2] - radius; // Reposition the sphere
        sphereVelocity.x *= -dampingFactor;
        if (std::abs(sphereVelocity.x) < minBounceSpeed)
        {
            sphereVelocity.x = 0.0f;
        }
    }

    // Check for collisions with the walls at the beginning of the main section
    if (spherePosition.z <= bounds[3] + radius)
    {
        spherePosition.z = bounds[3] + radius; // Reposition the sphere
        sphereVelocity.z *= -dampingFactor;
        if (std::abs(sphereVelocity.z) < minBounceSpeed)
        {
            sphereVelocity.z = 0.0f;
        }
    }
}

void drawTrail()
{
    glUseProgram(shaderProgram);

    for (size_t i = 0; i < trailPositions.size(); ++i) 
    {
        glm::vec3 position = trailPositions[i];
        float scale = 0.5f * (float)i / maxTrailLength; // Larger spheres for newer positions, smaller for older
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f - (float)i / maxTrailLength); // Fade the color

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(scale));

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

        // Set the color
        GLuint colorLoc = glGetUniformLocation(shaderProgram, "color");
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

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update trail positions
    if (trailPositions.size() >= maxTrailLength)
    {
        trailPositions.erase(trailPositions.begin()); // Remove the oldest position if we exceed the max trail length
    }
    trailPositions.push_back(spherePosition); // Add the current position to the trail

    drawGround();
    drawWalls();
    drawTrail(); // Draw the trail before drawing the sphere
    drawCircle();
    drawCylinder();

    for (int i = 0; i < subSteps; ++i)
    {
        spherePosition.y += sphereVelocity.y / subSteps;
        spherePosition.x += sphereVelocity.x / subSteps;
        spherePosition.z += sphereVelocity.z / subSteps;

        sphereVelocity *= pow(friction, 1.0f / subSteps); // Apply friction per sub-step

        sphereVelocity.y -= gravity / subSteps;

        // Check if the sphere is above the ground
        if (spherePosition.y <= radius)
        {
            spherePosition.y = radius;
            sphereVelocity.y *= -dampingFactor;
            if (std::abs(sphereVelocity.y) < minBounceSpeed)
            {
                sphereVelocity.y = 0.0f;
            }
        }

        // Check for wall collisions
        checkSphereBounds();
    }

    // Check if the sphere entered the hole
    if (checkHoleCollision())
    {
        sphereVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    drawSphere();
    drawPowerGauge();

    if (levelTransition && glfwGetTime() - endTime > levelTransitionDelay)
    {
        // Wait for the transition delay
        levelTransition = false;
        currentCourse = (currentCourse + 1) % 2; // Move to the next course
        initialSpherePosition = currentCourse == 0 ? glm::vec3(0.0f, radius, 0.0f) : glm::vec3(-5.0f, radius, -5.0f);
        spherePosition = initialSpherePosition;
        trailPositions.clear(); // Clear the trail
        setupGround(); // Reload ground for the new course
        setupWalls(); // Reload walls for the new course
        showEndText = false;
    }

    if (showEndText && glfwGetTime() - endTime > 3.0)
    {
        // Wait for 3 seconds
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
