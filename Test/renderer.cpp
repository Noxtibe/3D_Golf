#include "renderer.h"
#include <iostream>
#include <gtc/type_ptr.hpp>
#include <cmath>


void Renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer->mouseDown)
    {
        double deltaX = xpos - renderer->lastX;
        double deltaY = ypos - renderer->lastY;
        renderer->angleX += deltaY * 0.1f; // Ajustement de l'angle de rotation autour de l'axe X en fonction du mouvement vertical de la souris
        renderer->angleY += deltaX * 0.1f; // Ajustement de l'angle de rotation autour de l'axe Y en fonction du mouvement horizontal de la souris
        renderer->lastX = xpos;
        renderer->lastY = ypos;
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->zoom -= yoffset * 0.1f; // Ajustement du zoom en fonction du défilement de la molette
    if (renderer->zoom < 1.0f) // Limitation du zoom minimum
        renderer->zoom = 1.0f;
}

Renderer::Renderer()
{
    angleX = 0.0f;
    angleY = 0.0f;
    zoom = 5.0f;
    lastX = 0.0;
    lastY = 0.0;
    mouseDown = false;
}

Renderer::~Renderer()
{
    glfwTerminate();
}

void Renderer::setupCallbacks()
{
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

void Renderer::init(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    setupCallbacks();

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    sphere = Sphere(); // Correction de l'initialisation de l'objet Sphere
}

void Renderer::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glm::mat4 view = glm::lookAt(glm::vec3(zoom * sin(angleX) * cos(angleY), zoom * cos(angleX), zoom * sin(angleX) * sin(angleY)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(glm::value_ptr(projection * view));
    glColor3f(1.0f, 1.0f, 1.0f); // Blanc
    sphere.draw();
    glPopMatrix();

    glfwSwapBuffers(window);
}

GLFWwindow* Renderer::getWindow()
{
    return window;
}
