#include "sphere.h"
#include <GL/glew.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>

Sphere::Sphere() 
{
    setupMesh();
}

Sphere::~Sphere() 
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

Sphere& Sphere::operator=(const Sphere& other)
{
    if (this != &other) // Vérifie si l'objet n'est pas le même que celui passé en paramètre
    {
        copyMesh(other); // Copie les données du maillage de l'autre sphère
    }
    return *this;
}

void Sphere::setupMesh() 
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    float x, y, z, xy;                              // Vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // Vertex normal

    float sectorStep = 2 * glm::pi<float>() / sectorCount;
    float stackStep = glm::pi<float>() / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) 
    {
        stackAngle = glm::pi<float>() / 2 - i * stackStep;       // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);                          // r * cos(u)
        z = radius * sinf(stackAngle);                           // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) 
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    std::vector<GLfloat> sphereVertices;
    for (int i = 0; i < vertices.size(); i += 6) 
    {
        sphereVertices.push_back(vertices[i]);
        sphereVertices.push_back(vertices[i + 1]);
        sphereVertices.push_back(vertices[i + 2]);
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), &sphereVertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Sphere::copyMesh(const Sphere& other)
{
    // Supprimer les anciens identifiants de vertex array et vertex buffer object
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

    // Copier les données du maillage de l'autre sphère
    vao = other.vao;
    vbo = other.vbo;
}

void Sphere::draw() 
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (sectorCount + 1) * 2 * stackCount);
    glBindVertexArray(0);
}
