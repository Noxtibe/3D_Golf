#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include "sphere.h"

class Renderer 
{

public:

    Renderer();
    ~Renderer();
    void init(int width, int height, const char* title);
    void render();
    GLFWwindow* getWindow();

private:

    GLFWwindow* window;
    Sphere sphere;

    float angleX;
    float angleY;
    float zoom;
    double lastX;
    double lastY;

    bool mouseDown;

    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void setupCallbacks();
};
