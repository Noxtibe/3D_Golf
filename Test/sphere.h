#pragma once
#include <GL/glew.h>

class Sphere 
{

public:

    Sphere();
    ~Sphere();

    Sphere& operator=(const Sphere& other);

    void draw();

private:

    GLuint vao;
    GLuint vbo;

    const int sectorCount = 36;
    const int stackCount = 18;
    const float radius = 1.0f;

    void setupMesh();
    void copyMesh(const Sphere& other); // Fonction pour copier les données du maillage
};

